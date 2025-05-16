from django.urls import path
from app import views
from app.views_auth import RegisterView, LogoutView
from rest_framework_simplejwt.views import TokenObtainPairView, TokenRefreshView
from .views import APIPingView

urlpatterns = [
    # Маршруты для работы с городами
    path('cities/', views.CityListView.as_view(), name='city-list'),
    
    # Маршруты для работы с точками
    path('points/', views.Point3DListView.as_view(), name='point3d-list'),
    path('obstacles/', views.Obstacle3DListView.as_view(), name='obstacle3d-list'),
    
    # Маршрут для проверки состояния API
    path('ping/', APIPingView.as_view(), name='api-ping'), 

    # Маршрут для регистрации
    path('register/', RegisterView.as_view(), name='register'),
    
    # Маршруты для получения токенов JWT
    path('token/', TokenObtainPairView.as_view(), name='token_obtain_pair'),
    path('token/refresh/', TokenRefreshView.as_view(), name='token_refresh'),
    
    # Маршрут для логаута
    path('logout/', LogoutView.as_view(), name='logout'),
]
