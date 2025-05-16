from django.contrib.gis.db import models as gis_models
from django.db import models
import uuid
from django.contrib.gis.geos import GEOSGeometry

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
        db_table = 'points_new'
        managed = False  # если таблица уже создана в БД

class Obstacle3D(models.Model):
    guid = models.UUIDField(primary_key=True, editable=False)
    type = models.CharField(max_length=100)
    geom = gis_models.GeometryField(srid=4326)
    elev = models.FloatField(null=True, blank=True)

    class Meta:
        db_table = 'obstacles'

    def __str__(self):
        return f"{self.type} ({self.guid})"

    def save(self, *args, **kwargs):
        if self.geom and hasattr(self.geom, 'z'):
            self.elev = self.geom.z or 0.0
        super().save(*args, **kwargs)