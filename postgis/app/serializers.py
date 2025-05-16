# serializers.py
from rest_framework import serializers
from django.contrib.gis.geos import Point
from .models import City, Point3D
from .models import Obstacle3D

class CitySerializer(serializers.ModelSerializer):
    distance = serializers.SerializerMethodField()

    class Meta:
        model = City
        fields = [field.name for field in City._meta.fields] + ['distance']

    def get_distance(self, obj):
        if hasattr(obj, 'distance'):
            return round(obj.distance.km, 2)
        return None

class Point3DSerializer(serializers.ModelSerializer):
    distance = serializers.SerializerMethodField()

    class Meta:
        model = Point3D
        fields = [field.name for field in Point3D._meta.fields] + ['distance']

    def get_distance(self, obj):
        if hasattr(obj, 'distance') and obj.distance is not None:
            return round(obj.distance, 2)
        return None

class Obstacle3DSerializer(serializers.ModelSerializer):
    distance = serializers.SerializerMethodField()
    length = serializers.SerializerMethodField()
    geom_type = serializers.SerializerMethodField()

    class Meta:
        model = Obstacle3D
        fields = [field.name for field in Obstacle3D._meta.fields] + ['distance', 'length', 'geom_type']

    def get_distance(self, obj):
        if hasattr(obj, 'distance') and obj.distance is not None:
            return round(obj.distance, 2)
        return None

    def get_length(self, obj):
        if hasattr(obj, 'length_km') and obj.length_km is not None:
            return round(obj.length_km, 2)
        return None

    def get_geom_type(self, obj):
        if obj.geom:
            return obj.geom.geom_type
        return None
