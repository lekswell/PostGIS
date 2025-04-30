from rest_framework import serializers
from .models import City, Point3D

class CitySerializer(serializers.ModelSerializer):
    class Meta:
        model = City
        fields = '__all__'

class Point3DSerializer(serializers.ModelSerializer):
    class Meta:
        model = Point3D
        fields = '__all__'
