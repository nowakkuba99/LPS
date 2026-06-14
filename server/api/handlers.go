package api

import (
	"encoding/json"
	"net/http"

	"github.com/go-chi/chi/v5"

	"lps/jobs"
)

type Handler struct {
	jobs *jobs.Manager
}

func (h *Handler) CreateSimulation(w http.ResponseWriter, r *http.Request) {
	var cfg jobs.Config
	if err := json.NewDecoder(r.Body).Decode(&cfg); err != nil {
		jsonError(w, "invalid JSON body", http.StatusBadRequest)
		return
	}
	job := h.jobs.Submit(cfg)
	writeJSON(w, http.StatusCreated, job.View(false))
}

func (h *Handler) ListSimulations(w http.ResponseWriter, r *http.Request) {
	all := h.jobs.List()
	views := make([]jobs.JobView, len(all))
	for i, j := range all {
		views[i] = j.View(false)
	}
	writeJSON(w, http.StatusOK, views)
}

func (h *Handler) GetSimulation(w http.ResponseWriter, r *http.Request) {
	id := chi.URLParam(r, "id")
	job, ok := h.jobs.Get(id)
	if !ok {
		jsonError(w, "not found", http.StatusNotFound)
		return
	}
	writeJSON(w, http.StatusOK, job.View(true))
}

func (h *Handler) CancelSimulation(w http.ResponseWriter, r *http.Request) {
	id := chi.URLParam(r, "id")
	if !h.jobs.Cancel(id) {
		jsonError(w, "not found", http.StatusNotFound)
		return
	}
	w.WriteHeader(http.StatusNoContent)
}

func writeJSON(w http.ResponseWriter, status int, v any) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	json.NewEncoder(w).Encode(v)
}

func jsonError(w http.ResponseWriter, msg string, status int) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	json.NewEncoder(w).Encode(map[string]string{"error": msg})
}
