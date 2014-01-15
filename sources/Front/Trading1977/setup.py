
__author__ = "Eugene V. Palchukovsky"
__email__ = "eugene@palchukovsky.com"
__date__ = "2014/01/14 21:45:42"

from distutils.core import setup
import py2exe
import os

###############################################################################

def addPathTree(base, path, skip_dirs=['.svn', '.git']):
    path = os.path.join(base, path)
    result = []
    for root, dirs, files in os.walk(os.path.join(path)):
        sampleList = []
        for skipDir in skip_dirs:
            if skipDir in dirs:
                dirs.remove(skipDir)
        if files:
            for filename in files:
                sampleList.append(os.path.join(root, filename))
        if sampleList:
            result.append((
                root.replace(
                    base + os.sep if base else '',
                    '',
                    1
                ),
                sampleList
            ))
    return result

###############################################################################

py2exeOptions = {
    'py2exe': {
        'compressed': 1,
        'optimize': 2,
        'ascii': 1,
        'bundle_files': 1,
        'dist_dir': 'setup',
        'packages': ['encodings'],
        'excludes': [
            'pywin',
            'pywin.debugger',
            'pywin.debugger.dbgcon',
            'pywin.dialogs',
            'pywin.dialogs.list',
            'Tkconstants',
            'Tkinter',
            'tcl'
        ],
        'dll_excludes': [
            'w9xpopen.exe',
            'MSVCR71.dll'],
        'includes': [
            'trdkfront.urls',
            'manage',
            'trdkfront.settings',
            'htmllib',
            'HTMLParser',
            'django.template.loaders.filesystem',
            'django.template.loaders.app_directories',
            'django.middleware.common',
            'django.middleware.clickjacking',
            'django.middleware.doc',
            'django.contrib.sessions.middleware',
            'django.contrib.messages.middleware',
            'django.contrib.messages.storage.fallback',
            'django.contrib.messages.context_processors',
            'django.contrib.auth',
            'django.contrib.auth.middleware',
            'django.contrib.auth.context_processors',
            'django.contrib.contenttypes',
            'django.contrib.sessions',
            'django.contrib.sessions.backends.db',
            'django.contrib.sessions.serializers',
            'django.contrib.sites',
            'django.contrib.admin',
            'django.core.cache.backends',
            'django.db.backends.sqlite3.base',
            'django.db.backends.sqlite3.introspection',
            'django.db.backends.sqlite3.creation',
            'django.db.backends.sqlite3.client',
            'django.template.defaulttags',
            'django.template.defaultfilters',
            'django.template.loader_tags',
            'django.contrib.admin.views.main',
            'django.core.context_processors',
            'django.contrib.auth.views',
            'django.contrib.auth.backends',
            'django.views.static',
            'django.contrib.admin.templatetags.admin_list',
            'django.contrib.admin.templatetags.admin_modify',
            'django.contrib.admin.templatetags.log',
            'django.conf.urls.shortcut',
            'django.views.defaults',
            'django.core.cache.backends.locmem',
            'django.templatetags.i18n',
            'django.views.i18n',
            'email.mime.audio',
            'email.mime.base',
            'email.mime.image',
            'email.mime.message',
            'email.mime.multipart',
            'email.mime.nonmultipart',
            'email.mime.text',
            'email.charset',
            'email.encoders',
            'email.errors',
            'email.feedparser',
            'email.generator',
            'email.header',
            'email.iterators',
            'email.message',
            'email.parser',
            'email.utils',
            'email.base64mime',
            'email.quoprimime']}
}
 
pythonPath = os.environ['PYTHONPATH'].split(';')[0]
 
djangoAdminPath \
    = os.path.normpath(pythonPath + '/lib/site-packages/django/contrib/admin')
py2exeDataFiles = []
 

py2exeDataFiles += addPathTree(djangoAdminPath, 'templates')
py2exeDataFiles += addPathTree(djangoAdminPath, 'media')

py2exeDataFiles += addPathTree('', 'db')
py2exeDataFiles += addPathTree('', 'templates')

py2exeDataFiles += addPathTree('', 'dashboard/templates')

setup(
  options = py2exeOptions,
  data_files = py2exeDataFiles,
  zipfile = None,
  console = ['trdkfront.py'])