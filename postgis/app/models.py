from django.contrib.gis.db import models as gis_models
from django.db import models

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
