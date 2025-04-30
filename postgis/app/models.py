from django.contrib.gis.db import models as gis_models
from django.db import models
import uuid

class City(models.Model):
    id = models.BigIntegerField(primary_key=True)
    city = models.CharField(max_length=50, null=True)
    lat = models.FloatField()
    lng = models.FloatField()
    capital = models.CharField(max_length=50, null=True)
    population = models.BigIntegerField()
    geom = gis_models.PointField(srid=4326)

    def __str__(self):
        return f"{self.id} - {self.capital}"
    class Meta:
        managed = False
        db_table = 'cities'


class Point3D(models.Model):
    guid = models.UUIDField(primary_key=True, default=uuid.uuid4, editable=False)
    geom = gis_models.GeometryField(srid=4326)  # Для 3D можно использовать GeometryField
    type = models.CharField(max_length=255)
    lon = models.FloatField()
    lat = models.FloatField()
    elev = models.FloatField()

    class Meta:
        db_table = 'points'
        managed = False  # если таблица уже создана в БД