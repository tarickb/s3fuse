#ifndef S3_SERVICES_MULTIPART_TRANSFER_H
#define S3_SERVICES_MULTIPART_TRANSFER_H

#include <iostream>
#include <vector>

#include "base/config.h"
#include "base/logger.h"
#include "threads/pool.h"

namespace s3
{
  namespace base
  {
    class request;
  }

  namespace services
  {
    template <class T>
    class multipart_transfer
    {
    public:
      typedef boost::function2<int, const boost::shared_ptr<base::request> &, T *> transfer_part_fn;
      typedef boost::function2<int, const boost::shared_ptr<base::request> &, T *> retry_part_fn;

      inline multipart_transfer(
        const std::vector<T> &parts,
        const transfer_part_fn &on_transfer_part,
        const retry_part_fn &on_retry_part,
        int max_retries = -1,
        int max_parts_in_progress = -1)
        : _on_transfer_part(on_transfer_part),
          _on_retry_part(on_retry_part)
      {
        _parts.resize(parts.size());

        for (size_t i = 0; i < parts.size(); i++) {
          _parts[i].part = parts[i];
          _parts[i].id = i;
        }

        _max_retries = (max_retries == -1) ? base::config::get_max_transfer_retries() : max_retries;
        _max_parts_in_progress = (max_parts_in_progress == -1) ? base::config::get_max_parts_in_progress() : max_parts_in_progress;
      }

      int process()
      {
        size_t last_part = 0;
        std::list<transfer_part *> parts_in_progress;
        int r = 0;

        for (last_part = 0; last_part < std::min(_max_parts_in_progress, _parts.size()); last_part++) {
          transfer_part *part = &_parts[last_part];

          part->handle = threads::pool::post(
            threads::PR_REQ_1, 
            bind(_on_transfer_part, _1, &part->part),
            0 /* don't retry on timeout since we handle that here */);

          parts_in_progress.push_back(part);
        }

        while (!parts_in_progress.empty()) {
          transfer_part *part = parts_in_progress.front();
          int part_r;

          parts_in_progress.pop_front();
          part_r = part->handle->wait();

          if (part_r) {
            S3_LOG(LOG_DEBUG, "multipart_transfer::process", "part %i returned status %i.\n", part->id, part_r);

            if ((part_r == -EAGAIN || part_r == -ETIMEDOUT) && part->retry_count < _max_retries) {
              part->handle = threads::pool::post(
                threads::PR_REQ_1, 
                bind(_on_retry_part, _1, &part->part),
                0);

              part->retry_count++;

              parts_in_progress.push_back(part);
            } else {
              if (r == 0) // only save the first non-successful return code
                r = part_r;
            }
          }

          // keep collecting parts until we have nothing left pending
          // if one part fails, keep going but stop posting new parts

          if (r == 0 && last_part < _parts.size()) {
            part = &_parts[last_part++];

            part->handle = threads::pool::post(
              threads::PR_REQ_1, 
              bind(_on_transfer_part, _1, &part->part),
              0);

            parts_in_progress.push_back(part);
          }
        }

        return r;
      }

    private:
      struct transfer_part
      {
        int id;
        int retry_count;
        threads::wait_async_handle::ptr handle;

        T part;

        inline transfer_part()
          : id(-1),
            retry_count(0)
        {
        }
      };

      std::vector<transfer_part> _parts;

      transfer_part_fn _on_transfer_part;
      retry_part_fn _on_retry_part;

      int _max_retries;
      size_t _max_parts_in_progress;
    };
  }
}

#endif
