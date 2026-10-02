importScripts('https://storage.googleapis.com/workbox-cdn/releases/7.0.0/workbox-sw.js');
workbox.routing.setDefaultHandler(new workbox.strategies.StaleWhileRevalidate());
