
__author__ = "Eugene V. Palchukovsky"
__email__ = "eugene@palchukovsky.com"
__date__ = "2014/01/08 22:02:29"

from django.conf.urls import patterns, include, url
from django.conf import settings
from django.contrib import admin

from dashboard import views as dashboard
from services import views as services

admin.autodiscover()

urlpatterns = patterns(
    '',
    url(r'^admin/', include(admin.site.urls)),
    url(
        r'^media/(?P<path>.*)$',
        'django.views.static.serve',
        {'document_root': settings.MEDIA_ROOT}),
    url(r'^$', dashboard.show),
    url(r'^services/getState', services.getState),
    url(r'^services/addStrategy', services.addStrategy),
    url(r'^services/deleteStrategy', services.deleteStrategy),
    url(r'^services/buy$', services.openLongPosition),
    url(r'^services/sell$', services.openShortPosition),
    url(r'^services/closePosition$', services.closePosition),
    url(r'^services/closeAllPositions$', services.closeAllPositions))
