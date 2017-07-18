"""
/*******************************************************************************
 *   Created: 2016/02/07 20:32:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit, Builder
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/
"""

import lib
import settings
import version
import git

__author__ = 'Eugene V. Palchukovsky'
__copyright__ = 'Eugene V. Palchukovsky'
__email__ = 'eugene@palchukovsky.com'
__all__ = ['build_release', 'build_test']


def build_release():
    repo = get_repo()
    version.increase_number_of_release()
    build_sources()
    create_package()
    deploy()
    repo.commit()


def build_test():
    repo = get_repo()
    version.increase_number_of_build()
    build_sources()
    create_package()
    deploy()
    repo.commit()


def get_repo():
    repo = git.Repo(settings.ROOT_PATH)
    if repo.is_dirty():
        raise lib.LocalException('Repository is dirty, make it clean first.')
    return repo


def build_sources():
    pass


def create_package():
    pass


def deploy():
    pass


