import django_filters
from django.contrib.gis.geos import Point, Polygon
from django.contrib.gis.measure import D
from .models import City

class CityFilter(django_filters.FilterSet):
    capital = django_filters.CharFilter(field_name='capital', lookup_expr='iexact')
    limit = django_filters.NumberFilter(method='filter_limit')
    point = django_filters.CharFilter(method='filter_by_distance')  # Используем только point
    bbox = django_filters.CharFilter(method='filter_bbox')

    def filter_limit(self, queryset, name, value):
        return queryset[:value]

    def filter_by_distance(self, queryset, name, value):
        point_param = self.data.get("point")
        radius = self.data.get("radius")

        if point_param and radius:
            try:
                lng, lat = map(float, point_param.split(","))
                user_location = Point(lng, lat, srid=4326)
                return queryset.filter(geom__distance_lte=(user_location, D(km=float(radius))))
            except (ValueError, TypeError):
                pass
        return queryset

    def filter_bbox(self, queryset, name, value):
        try:
            lon1, lat1, lon2, lat2 = map(float, value.split(','))
            bbox = Polygon.from_bbox((lon1, lat1, lon2, lat2))
            bbox.srid = 4326
            return queryset.filter(geom__within=bbox)
        except Exception:
            return queryset.none()

    class Meta:
        model = City
        fields = ['capital']
