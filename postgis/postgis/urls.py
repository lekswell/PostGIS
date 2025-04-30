from django.contrib import admin
from django.urls import path, include
from rest_framework import permissions
from drf_yasg.views import get_schema_view
from drf_yasg import openapi

# Схема Swagger
schema_view = get_schema_view(
    openapi.Info(
        title="Cities and Points API",
        default_version='v1',
        description="API documentation for Cities and Points app",
    ),
    public=True,
    permission_classes=(permissions.AllowAny,),
    authentication_classes=[],  # можно явно указать если нужно
)


urlpatterns = [
    path('admin/', admin.site.urls),
    
    # Все маршруты для API (города, точки, регистрация, JWT)
    path('api/', include('app.urls')),  # Все маршруты находятся в app.urls

    # Добавляем Swagger UI
    path('swagger/', schema_view.with_ui('swagger', cache_timeout=0), name='swagger-ui'),
]
