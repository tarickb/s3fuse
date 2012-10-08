int fs::__rename_object(const request::ptr &req, const string &from, const string &to)
{
  int r;
  object::ptr obj, target_obj;

  ASSERT_NO_TRAILING_SLASH(from);
  ASSERT_NO_TRAILING_SLASH(to);

  obj = _object_cache->get(req, from);

  if (!obj)
    return -ENOENT;

  target_obj = _object_cache->get(req, to);

  if (target_obj) {
    if (
      (obj->get_type() == OT_DIRECTORY && target_obj->get_type() != OT_DIRECTORY) ||
      (obj->get_type() != OT_DIRECTORY && target_obj->get_type() == OT_DIRECTORY)
    )
      return -EINVAL;

    // TODO: this doesn't handle the case where "to" points to a directory
    // that isn't empty.

    r = __remove_object(req, to);

    if (r)
      return r;
  }

  invalidate_parent(from);
  invalidate_parent(to);

  if (obj->get_type() == OT_DIRECTORY)
    return rename_children(req, from, to);
  else {
    r = copy_file(req, from, to);

    if (r)
      return r;

    _object_cache->remove(from);
    return remove_object(req, obj->get_url());
  }
}

// TODO: invalidate parent when removing an object!
