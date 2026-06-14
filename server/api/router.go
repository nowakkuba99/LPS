package api

import (
	"net/http"

	"github.com/go-chi/chi/v5"
	"github.com/go-chi/chi/v5/middleware"

	"lps/jobs"
)

func NewRouter(m *jobs.Manager) http.Handler {
	r := chi.NewRouter()
	r.Use(middleware.Logger)
	r.Use(middleware.Recoverer)
	r.Use(corsMiddleware)

	h := &Handler{jobs: m}

	r.Route("/api", func(r chi.Router) {
		r.Post("/simulations", h.CreateSimulation)
		r.Get("/simulations", h.ListSimulations)
		r.Get("/simulations/{id}", h.GetSimulation)
		r.Delete("/simulations/{id}", h.CancelSimulation)
	})

	return r
}

func corsMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type")
		if r.Method == http.MethodOptions {
			w.WriteHeader(http.StatusNoContent)
			return
		}
		next.ServeHTTP(w, r)
	})
}
