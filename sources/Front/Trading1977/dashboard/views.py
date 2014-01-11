
__author__ = "Eugene V. Palchukovsky"
__email__ = "eugene@palchukovsky.com"
__date__ = "2014/01/08 22:06:13"

from django.shortcuts import render

def show(request):
    return render(request, 'index.html')
