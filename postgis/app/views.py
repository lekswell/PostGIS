from rest_framework import viewsets, filters
from django_filters.rest_framework import DjangoFilterBackend
from .models import City
from .serializers import CitySerializer
from .filters import CityFilter

class CityViewSet(viewsets.ReadOnlyModelViewSet):
    queryset = City.objects.all()
    serializer_class = CitySerializer
    filter_backends = [DjangoFilterBackend, filters.OrderingFilter, filters.SearchFilter]
    filterset_class = CityFilter
    search_fields = ['capital']
    ordering_fields = ['population']
