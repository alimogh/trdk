"""
/*******************************************************************************
 *   Created: 2016/02/07 16:34:40
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit, Builder
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/
"""

import os

__author__ = 'Eugene V. Palchukovsky'
__copyright__ = 'Eugene V. Palchukovsky'
__email__ = 'eugene@palchukovsky.com'

BUILDER_PATH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROOT_PATH = os.path.join(BUILDER_PATH, '..')
SOLUTION_PATH = os.path.join(ROOT_PATH, 'sources')
VERSION_PROJECT_PATH = os.path.join(SOLUTION_PATH, 'Version')

try:
    from settings_local import *
except Exception as ex:
    import sys

    sys.exit('Failed to load local settings: "{0}".'.format(ex))
