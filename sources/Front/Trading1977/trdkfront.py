
__author__ = "Eugene V. Palchukovsky"
__email__ = "eugene@palchukovsky.com"
__date__ = "2014/01/14 21:49:32"

import os

os.environ['DJANGO_SETTINGS_MODULE'] = "trdkfront.settings"
 
import cherrypy
import webbrowser
from django.core.handlers.wsgi import WSGIHandler
from django.contrib.staticfiles.handlers import StaticFilesHandler
 
if __name__ == "__main__":

    print 'To stop server close this window.'

    cherrypy.config.update({
        'environment': 'production',
        'log.error_file': 'site.log',
        'log.screen': False})
 
    cherrypy.tree.graft(StaticFilesHandler(WSGIHandler()), '/')
    cherrypy.server.socket_port = 8000
    cherrypy.server.socket_host = '127.0.0.1'

    cherrypy.engine.start()
    cherrypy.engine.block()
#    cherrypy.engine.start_with_callback(
#        webbrowser.open,
#        ('http://localhost:8000', ))
