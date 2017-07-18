#!/usr/bin/python
"""
/*******************************************************************************
 *   Created: 2016/02/07 16:38:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit, Builder
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/
"""

from builder import build_test, build_release
import sys
import getopt

__author__ = 'Eugene V. Palchukovsky'
__copyright__ = 'Eugene V. Palchukovsky'
__email__ = 'eugene@palchukovsky.com'
__all__ = []


def print_copyright():
    print 'Trading Robot Development Kit Node Builder'
    print 'Copyright: ' + __copyright__
    print 'E-mail: ' + __email__
    print 'URL: http://robotdk.com/'


def print_help():
    print_help()
    print('')
    print('Usage:')
    print("\tbuild.py command")
    print('Commands:')
    print("\t-h, --help - This help.")
    print("\t-b, --build=[test | release] - Build project.")


def main(argv):

    try:
        opts, args = getopt.getopt(
            argv,
            'hb:',
            ['help', 'build'])
    except getopt.GetoptError as ex:
        print(
            'Failed to get options: "{0}". Use "--help" for help.'.format(
                ex.message))
        return 1

    for opt, arg in opts:
        if opt in ('-h', 'help'):
            print_help()
            return 0
        elif opt in ("-b", "--build"):
            if arg == 'release':
                return build_release()
            elif arg == 'test':
                return build_test()
            else:
                print(
                    'Unknown build type "{0}". Use "--help" for help.'.format(
                        arg))
                print_help()
                return 1
        else:
            print(
                'Unknown option "{0}"="{1}". Use "--help" for help.'.format(
                    opt,
                    arg))
            return 1

    print_copyright()
    print('')
    print('No command. Use "--help" for help.')
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
