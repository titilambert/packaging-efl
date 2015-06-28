#ifndef _EIO_SUITE_H
#define _EIO_SUITE_H

#include <check.h>

void eio_test_monitor(TCase *tc);
void eio_model_test_file(TCase *tc);
void eio_model_test_monitor_add(TCase *tc);
void eio_test_file(TCase *tc);
void eio_test_xattr(TCase *tc);

#endif /*  _EIO_SUITE_H */
