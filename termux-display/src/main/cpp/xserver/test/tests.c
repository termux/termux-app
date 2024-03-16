#include <string.h>
#include "tests.h"
#include "tests-common.h"

int
main(int argc, char **argv)
{
    run_test(list_test);
    run_test(string_test);

#ifdef XORG_TESTS
    run_test(fixes_test);
    run_test(input_test);
    run_test(misc_test);
    run_test(signal_logging_test);
    run_test(touch_test);
    run_test(xfree86_test);
    run_test(xkb_test);
    run_test(xtest_test);

#ifdef RES_TESTS
    run_test(hashtabletest_test);
#endif

#ifdef LDWRAP_TESTS
    run_test(protocol_xchangedevicecontrol_test);

    run_test(protocol_xiqueryversion_test);
    run_test(protocol_xiquerydevice_test);
    run_test(protocol_xiselectevents_test);
    run_test(protocol_xigetselectedevents_test);
    run_test(protocol_xisetclientpointer_test);
    run_test(protocol_xigetclientpointer_test);
    run_test(protocol_xipassivegrabdevice_test);
    run_test(protocol_xiquerypointer_test);
    run_test(protocol_xiwarppointer_test);
    run_test(protocol_eventconvert_test);
    run_test(xi2_test);
#endif

#endif /* XORG_TESTS */

    return 0;
}
