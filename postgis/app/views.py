from rest_framework import generics, permissions
from drf_yasg.utils import swagger_auto_schema
from drf_yasg import openapi
from django.contrib.gis.geos import Point, Polygon
from django.contrib.gis.db.models.functions import Distance
from django.contrib.gis.measure import D
from django.db.models import F, Func, FloatField, ExpressionWrapper
from django.db.models.functions import Sqrt, Power
from .models import City, Point3D
from .serializers import CitySerializer, Point3DSerializer
from .filters import CityFilter, Point3DFilter
import math
from rest_framework.views import APIView
from rest_framework.response import Response
from rest_framework import status
from rest_framework import generics
from .models import Obstacle3D
from .serializers import Obstacle3DSerializer
from .filters import Obstacle3DFilter
from drf_yasg.utils import swagger_auto_schema
from drf_yasg import openapi
from django.contrib.gis.db.models.functions import Length

class APIPingView(APIView):
    def get(self, request):
        return Response({"message": "API is up and running"}, status=status.HTTP_200_OK)

class CityListView(generics.ListAPIView):
    serializer_class = CitySerializer
    filterset_class = CityFilter

    def get_queryset(self):
        queryset = City.objects.all()
        point_param = self.request.query_params.get("point")
        radius = self.request.query_params.get("radius")

        if point_param and radius:
            try:
                # Разбираем параметры
                parts = list(map(float, point_param.split(",")))
                lng, lat = parts[0], parts[1]

                if len(parts) == 3:
                    elev = parts[2]
                    user_location = Point(lng, lat, elev, srid=4326)
                else:
                    elev = 0.0
                    user_location = Point(lng, lat, srid=4326)
                if len(parts) == 3:
                    queryset = queryset.annotate(
                        distance=Distance("geom", user_location)
                    )
                else:
                    # Используем только 2D координаты для вычисления расстояния
                    queryset = queryset.annotate(
                        distance=Distance("geom", Point(lng, lat, srid=4326))
                    )

                # Фильтруем по радиусу
                queryset = queryset.filter(distance__lte=D(km=float(radius)))
            except Exception as e:
                print(f"Error during distance calculation: {e}")

        return queryset

    @swagger_auto_schema(
        operation_description="Retrieve a list of cities with optional filters.",
        manual_parameters=[
            openapi.Parameter('capital', openapi.IN_QUERY, description="Filter by capital (exact match)", type=openapi.TYPE_STRING),
            openapi.Parameter('limit', openapi.IN_QUERY, description="Limit the number of results", type=openapi.TYPE_INTEGER),
            openapi.Parameter('point', openapi.IN_QUERY, description="Center point for distance filter, format: 'lng,lat'", type=openapi.TYPE_STRING),
            openapi.Parameter('radius', openapi.IN_QUERY, description="Radius in kilometers", type=openapi.TYPE_NUMBER),
            openapi.Parameter('bbox', openapi.IN_QUERY, description="Bounding box filter, format: 'lat1,lon1,lat2,lon2'", type=openapi.TYPE_STRING),
            openapi.Parameter('elev', openapi.IN_QUERY, description="Elevation filter", type=openapi.TYPE_NUMBER)
        ]
    )
    def get(self, request, *args, **kwargs):
        return super().get(request, *args, **kwargs)

class Point3DListView(generics.ListAPIView):
    serializer_class = Point3DSerializer
    filterset_class = Point3DFilter
    # permission_classes = [permissions.IsAuthenticated]

    def get_queryset(self):
        queryset = Point3D.objects.all()
        point_param = self.request.query_params.get("point")
        radius = self.request.query_params.get("radius")

        if point_param:
            try:
                parts = list(map(float, point_param.split(",")))
                lng, lat = parts[0], parts[1]
                elev = parts[2] if len(parts) == 3 else 0.0

                user_location = Point(lng, lat, srid=4326)

                # 2D расстояние в километрах
                queryset = queryset.annotate(
                    dist2d_km=ExpressionWrapper(
                        Distance("geom", user_location) / 1000.0,
                        output_field=FloatField()
                    )
                )

                # Разница по высоте — тоже в километрах
                queryset = queryset.annotate(
                    dz_km=ExpressionWrapper(
                        (F("elev") - elev) / 1000.0,
                        output_field=FloatField()
                    )
                )

                # 3D расстояние в километрах
                queryset = queryset.annotate(
                    distance=ExpressionWrapper(
                        Sqrt(
                            Power(F("dist2d_km"), 2) + Power(F("dz_km"), 2)
                        ),
                        output_field=FloatField()
                    )
                )

                # Фильтрация по radius (в км)
                if radius:
                    queryset = queryset.filter(distance__lte=float(radius))

            except Exception as e:
                print(f"Error calculating 3D distance: {e}")

        return queryset


# class Point3DListView(generics.ListAPIView):
#     serializer_class = Point3DSerializer
#     filterset_class = Point3DFilter
#     permission_classes = [permissions.IsAuthenticated]

#     def get_queryset(self):
#         queryset = Point3D.objects.all()
#         point_param = self.request.query_params.get("point")
#         radius = self.request.query_params.get("radius")

#         if point_param:
#             try:
#                 parts = list(map(float, point_param.split(",")))
#                 lng, lat = parts[0], parts[1]
#                 user_location = Point(lng, lat, srid=4326)

#                 # Всегда аннотируем distance, если есть point
#                 queryset = queryset.annotate(distance=Distance("geom", user_location))

#                 # Если есть radius — фильтруем по нему
#                 if radius:
#                     queryset = queryset.filter(distance__lte=D(km=float(radius)))

#             except Exception as e:
#                 print(f"Error during distance calculation: {e}")

#         return queryset

    # def get_queryset(self):
    #     queryset = Point3D.objects.all()
    #     point_param = self.request.query_params.get("point")
    #     radius = self.request.query_params.get("radius")

    #     if point_param and radius:
    #         try:
    #             # Разбираем параметры
    #             parts = list(map(float, point_param.split(",")))
    #             lng, lat = parts[0], parts[1]

    #             if len(parts) == 3:
    #                 elev = parts[2]
    #                 user_location = Point(lng, lat, elev, srid=4326)  # 3D точка
    #             else:
    #                 elev = 0.0
    #                 user_location = Point(lng, lat, srid=4326)  # 2D точка, высота считается равной 0

    #             # Если передана высота, считаем 3D расстояние
    #             if len(parts) == 3:
    #                 queryset = queryset.annotate(
    #                     distance=Distance("geom", user_location)
    #                 )
    #             else:
    #                 # Используем только 2D координаты для вычисления расстояния
    #                 queryset = queryset.annotate(
    #                     distance=Distance("geom", Point(lng, lat, srid=4326))
    #                 )

    #             # Фильтруем по радиусу
    #             queryset = queryset.filter(distance__lte=D(km=float(radius)))
    #         except Exception as e:
    #             print(f"Error during distance calculation: {e}")

    #     return queryset

    @swagger_auto_schema(
        operation_description="Retrieve a list of 3D points with optional filters.",
        manual_parameters=[
            openapi.Parameter('type', openapi.IN_QUERY, description="Filter by type (exact match)", type=openapi.TYPE_STRING),
            openapi.Parameter('limit', openapi.IN_QUERY, description="Limit the number of results", type=openapi.TYPE_INTEGER),
            openapi.Parameter('point', openapi.IN_QUERY, description="Center point for distance filter, format: 'lng,lat,elev'", type=openapi.TYPE_STRING),
            openapi.Parameter('radius', openapi.IN_QUERY, description="Radius in kilometers", type=openapi.TYPE_NUMBER),
            openapi.Parameter('bbox', openapi.IN_QUERY, description="Bounding box filter, format: 'lat1,lon1,lat2,lon2'", type=openapi.TYPE_STRING),
            openapi.Parameter('elev', openapi.IN_QUERY, description="Elevation filter", type=openapi.TYPE_NUMBER)
        ]
    )
    def get(self, request, *args, **kwargs):
        return super().get(request, *args, **kwargs)

class Obstacle3DListView(generics.ListAPIView):
    serializer_class = Obstacle3DSerializer
    filterset_class = Obstacle3DFilter
    queryset = Obstacle3D.objects.all()

    def get_queryset(self):
        queryset = Obstacle3D.objects.all()
        point_param = self.request.query_params.get("point")
        radius = self.request.query_params.get("radius")

        if point_param:
            try:
                parts = list(map(float, point_param.split(",")))
                lng, lat = parts[0], parts[1]
                elev = parts[2] if len(parts) == 3 else 0.0

                user_location = Point(lng, lat, srid=4326)

                queryset = queryset.annotate(
                    dist2d_km=ExpressionWrapper(
                        Distance("geom", user_location) / 1000.0,
                        output_field=FloatField()
                    ),
                    dz_km=ExpressionWrapper(
                        (F("elev") - elev) / 1000.0,
                        output_field=FloatField()
                    ),
                    distance=ExpressionWrapper(
                        Sqrt(
                            Power(F("dist2d_km"), 2) + Power(F("dz_km"), 2)
                        ),
                        output_field=FloatField()
                    )
                )

                # длина линий (в км)
                queryset = queryset.annotate(
                    length_km=ExpressionWrapper(
                        Length("geom") / 1000.0,
                        output_field=FloatField()
                    )
                )

                if radius:
                    queryset = queryset.filter(distance__lte=float(radius))

            except Exception as e:
                print(f"Error calculating distance: {e}")

        return queryset

    @swagger_auto_schema(
        operation_description="Get 3D obstacles with optional filters.",
        manual_parameters=[
            openapi.Parameter('type', openapi.IN_QUERY, description="Filter by type", type=openapi.TYPE_STRING),
            openapi.Parameter('point', openapi.IN_QUERY, description="lng,lat,elev", type=openapi.TYPE_STRING),
            openapi.Parameter('radius', openapi.IN_QUERY, description="Radius in kilometers", type=openapi.TYPE_NUMBER),
            openapi.Parameter('bbox', openapi.IN_QUERY, description="lat1,lon1,lat2,lon2", type=openapi.TYPE_STRING),
            openapi.Parameter('elev', openapi.IN_QUERY, description="Elevation filter", type=openapi.TYPE_NUMBER),
            openapi.Parameter('limit', openapi.IN_QUERY, description="Limit results", type=openapi.TYPE_INTEGER),
        ]
    )
    def get(self, request, *args, **kwargs):
        return super().get(request, *args, **kwargs)
