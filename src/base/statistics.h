#ifndef S3_BASE_STATISTICS_H
#define S3_BASE_STATISTICS_H

#include <stdarg.h>

#include <fstream>
#include <stdexcept>
#include <string>
#include <boost/lexical_cast.hpp>

namespace s3
{
  namespace base
  {
    class statistics
    {
    public:
      static void init();

      inline static void write(const std::string &target = "")
      {
        if (target.empty()) {
          write(&std::cout);
        } else {
          std::ofstream f;

          f.open(target.c_str(), std::ofstream::trunc);

          if (!f.good())
            throw std::runtime_error("cannot open statistics target file for write");

          write(&f);

          f.close();
        }
      }

      template <class T>
      inline static void post(const std::string &id, T tag, const char *format, ...)
      {
        const size_t BUF_LEN = 256;

        va_list args;
        char buf[BUF_LEN];

        va_start(args, format);
        vsnprintf(buf, BUF_LEN, format, args);
        va_end(args);

        post(id + "_" + boost::lexical_cast<std::string>(tag), buf);
      }

    private:
      static void post(const std::string &id, const std::string &stats);
      static void write(std::ostream *output);
    };
  }
}

#endif
