from rest_framework import generics, permissions
from drf_yasg.utils import swagger_auto_schema
from drf_yasg import openapi
from .models import City, Point3D
from .serializers import CitySerializer, Point3DSerializer
from .filters import CityFilter, Point3DFilter

# Параметр Authorization для Swagger UI
token_param = openapi.Parameter(
    'Authorization',
    openapi.IN_HEADER,
    description="JWT token, format: Bearer <token>",
    type=openapi.TYPE_STRING
)

class CityListView(generics.ListAPIView):
    queryset = City.objects.all()
    serializer_class = CitySerializer
    filterset_class = CityFilter

    @swagger_auto_schema(
        operation_description="Retrieve a list of cities with optional filters.",
        manual_parameters=[
            openapi.Parameter('capital', openapi.IN_QUERY, description="Filter by capital (exact match)", type=openapi.TYPE_STRING),
            openapi.Parameter('limit', openapi.IN_QUERY, description="Limit the number of results", type=openapi.TYPE_INTEGER),
            openapi.Parameter('point', openapi.IN_QUERY, description="Center point for distance filter, format: 'lng,lat'", type=openapi.TYPE_STRING),
            openapi.Parameter('radius', openapi.IN_QUERY, description="Radius in meters for distance filter", type=openapi.TYPE_NUMBER),
            openapi.Parameter('bbox', openapi.IN_QUERY, description="Bounding box filter, format: 'lat1,lon1,lat2,lon2'", type=openapi.TYPE_STRING),
        ]
    )
    def get(self, request, *args, **kwargs):
        return super().get(request, *args, **kwargs)

class Point3DListView(generics.ListAPIView):
    queryset = Point3D.objects.all()
    serializer_class = Point3DSerializer
    filterset_class = Point3DFilter
    permission_classes = [permissions.IsAuthenticated]

    @swagger_auto_schema(
        operation_description="Retrieve a list of 3D points with optional filters.",
        manual_parameters=[
            openapi.Parameter('type', openapi.IN_QUERY, description="Filter by type (exact match)", type=openapi.TYPE_STRING),
            openapi.Parameter('limit', openapi.IN_QUERY, description="Limit the number of results", type=openapi.TYPE_INTEGER),
            openapi.Parameter('point', openapi.IN_QUERY, description="Center point for distance filter, format: 'lng,lat'", type=openapi.TYPE_STRING),
            openapi.Parameter('radius', openapi.IN_QUERY, description="Radius in meters for distance filter", type=openapi.TYPE_NUMBER),
            openapi.Parameter('bbox', openapi.IN_QUERY, description="Bounding box filter, format: 'lat1,lon1,lat2,lon2'", type=openapi.TYPE_STRING),
        ]
    )
    def get(self, request, *args, **kwargs):
        return super().get(request, *args, **kwargs)
