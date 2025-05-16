import django_filters
from django.contrib.gis.geos import Point, Polygon
from django.contrib.gis.measure import D
from .models import City, Point3D, Obstacle3D

class DistanceFilterMixin:
    def filter_limit(self, queryset, name, value):
        return queryset[:value] if value else queryset

    def filter_by_distance(self, queryset, name, value):
        point_param = self.data.get("point")
        radius = self.data.get("radius")

        if point_param and radius:
            try:
                lng, lat, *_ = map(float, point_param.split(","))
                user_location = Point(lng, lat, srid=4326)
                return queryset.filter(geom__distance_lte=(user_location, D(km=float(radius))))
            except Exception:
                pass
        return queryset

    def filter_bbox(self, queryset, name, value):
        try:
            lat1, lon1, lat2, lon2 = map(float, value.split(','))
            bbox = Polygon.from_bbox((lon1, lat1, lon2, lat2))
            bbox.srid = 4326
            return queryset.filter(geom__within=bbox)
        except Exception:
            return queryset.none()

    def filter_by_elevation(self, queryset, name, value):
        try:
            elev_offset = 0.0
            point_param = self.data.get("point")

            if point_param:
                parts = list(map(float, point_param.split(',')))
                if len(parts) == 3:
                    elev_offset = parts[2]  # elev из точки

            target_elev = elev_offset + float(value)
            return queryset.filter(elev__lte=target_elev)
        except Exception:
            return queryset

class CityFilter(django_filters.FilterSet, DistanceFilterMixin):
    capital = django_filters.CharFilter(field_name='capital', lookup_expr='iexact')
    point = django_filters.CharFilter(method='filter_by_distance')
    radius = django_filters.NumberFilter(method='filter_by_distance')
    bbox = django_filters.CharFilter(method='filter_bbox')
    elev = django_filters.NumberFilter(method='filter_by_elevation')
    limit = django_filters.NumberFilter(method='filter_limit')

    class Meta:
        model = City
        fields = ['capital']

class Point3DFilter(django_filters.FilterSet, DistanceFilterMixin):
    type = django_filters.CharFilter(field_name='type', lookup_expr='iexact')
    point = django_filters.CharFilter(method='filter_by_distance')
    radius = django_filters.NumberFilter(method='filter_by_distance')
    bbox = django_filters.CharFilter(method='filter_bbox')
    elev = django_filters.NumberFilter(method='filter_by_elevation')
    limit = django_filters.NumberFilter(method='filter_limit')

    class Meta:
        model = Point3D
        fields = ['type']

class Obstacle3DFilter(django_filters.FilterSet):
    type = django_filters.CharFilter(field_name='type', lookup_expr='iexact')
    point = django_filters.CharFilter(method='filter_by_distance')
    radius = django_filters.NumberFilter(method='filter_by_distance')
    bbox = django_filters.CharFilter(method='filter_bbox')
    elev = django_filters.NumberFilter(method='filter_by_elevation')
    limit = django_filters.NumberFilter(method='filter_limit')

    class Meta:
        model = Obstacle3D
        fields = ['type']

    def filter_limit(self, queryset, name, value):
        return queryset[:value] if value else queryset

    def filter_by_distance(self, queryset, name, value):
        point_param = self.data.get("point")
        radius = self.data.get("radius")

        if point_param and radius:
            try:
                lng, lat, *_ = map(float, point_param.split(","))
                user_location = Point(lng, lat, srid=4326)
                return queryset.filter(geom__distance_lte=(user_location, D(km=float(radius))))
            except Exception:
                pass
        return queryset

    def filter_bbox(self, queryset, name, value):
        try:
            lat1, lon1, lat2, lon2 = map(float, value.split(','))
            bbox = Polygon.from_bbox((lon1, lat1, lon2, lat2))
            bbox.srid = 4326
            return queryset.filter(geom__within=bbox)
        except Exception:
            return queryset.none()

    def filter_by_elevation(self, queryset, name, value):
        try:
            elev_offset = 0.0
            point_param = self.data.get("point")

            if point_param:
                parts = list(map(float, point_param.split(',')))
                if len(parts) == 3:
                    elev_offset = parts[2]

            target_elev = elev_offset + float(value)
            return queryset.filter(elev__lte=target_elev)
        except Exception:
            return queryset
