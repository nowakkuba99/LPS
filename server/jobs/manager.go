package jobs

import (
	"fmt"
	"sync"
	"sync/atomic"
	"time"
)

// Manager creates and tracks simulation jobs.
type Manager struct {
	binaryPath string
	mu         sync.RWMutex
	jobs       map[string]*Job
	counter    atomic.Uint64
}

func NewManager(binaryPath string) *Manager {
	return &Manager{
		binaryPath: binaryPath,
		jobs:       make(map[string]*Job),
	}
}

func (m *Manager) Submit(cfg Config) *Job {
	if cfg.ExcitationType == "" {
		cfg.ExcitationType = "wave_mix"
	}
	if cfg.TotalSteps <= 0 {
		cfg.TotalSteps = 30000
	}

	id := fmt.Sprintf("sim_%d_%d", time.Now().UnixNano(), m.counter.Add(1))
	job := newJob(id, cfg)

	m.mu.Lock()
	m.jobs[id] = job
	m.mu.Unlock()

	go m.run(job)
	return job
}

func (m *Manager) Get(id string) (*Job, bool) {
	m.mu.RLock()
	defer m.mu.RUnlock()
	j, ok := m.jobs[id]
	return j, ok
}

func (m *Manager) List() []*Job {
	m.mu.RLock()
	defer m.mu.RUnlock()
	out := make([]*Job, 0, len(m.jobs))
	for _, j := range m.jobs {
		out = append(out, j)
	}
	return out
}

func (m *Manager) Cancel(id string) bool {
	m.mu.RLock()
	j, ok := m.jobs[id]
	m.mu.RUnlock()
	if !ok {
		return false
	}
	j.Cancel()
	return true
}
