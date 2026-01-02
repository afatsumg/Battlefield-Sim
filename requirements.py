import pandas as pd
import numpy as np
from abc import ABC, abstractmethod

class Requirement(ABC):
    """Abstract Base Class for all Fusion Requirements."""
    def __init__(self, srid, threshold):
        self.srid = srid
        self.threshold = threshold

    @abstractmethod
    def validate(self, df) -> dict:
        """Must return a dict with: pass (bool), value (float), and detail (str)."""
        pass

# --- Concrete Implementations ---

class AccuracyRequirement(Requirement):
    """REQ-001: Steady-state average error."""
    def validate(self, df):
        steady_state = df.iloc[100:]['error_m']
        avg_err = steady_state.mean()
        is_pass = avg_err < self.threshold
        return {
            "srid": self.srid,
            "pass": is_pass,
            "val": round(avg_err, 2),
            "detail": f"Average Error < {self.threshold}m"
        }

class StabilityRequirement(Requirement):
    """REQ-002: Maximum peak error (No spikes)."""
    def validate(self, df):
        steady_state = df.iloc[100:]['error_m']
        max_err = steady_state.max()
        is_pass = max_err < self.threshold
        return {
            "srid": self.srid,
            "pass": is_pass,
            "val": round(max_err, 2),
            "detail": f"Peak Error < {self.threshold}m"
        }

class ConvergenceRequirement(Requirement):
    """REQ-003: Time to settle below 20m."""
    def validate(self, df):
        # Time step is 0.1s
        target = 20.0
        # Find first index where error < target
        below_target = df[df['error_m'] < target].index
        conv_time = below_target[0] * 0.1 if not below_target.empty else 99.9
        is_pass = conv_time < self.threshold
        return {
            "srid": self.srid,
            "pass": is_pass,
            "val": round(conv_time, 2),
            "detail": f"Convergence Time < {self.threshold}s"
        }

# --- The Orchestrator ---

class FusionValidator:
    """The Engine that runs all registered requirements."""
    def __init__(self):
        # Scale by simply adding new Requirement objects to this list
        self.requirements = [
            AccuracyRequirement("SRID-FUS-001", threshold=25.0),
            StabilityRequirement("SRID-FUS-002", threshold=100.0),
            ConvergenceRequirement("SRID-FUS-003", threshold=10.0)
        ]

    def verify_scenario(self, df):
        if len(df) < 150:
            return {"status": "INCOMPLETE", "checks": {}}

        results = {}
        for req in self.requirements:
            results[req.srid] = req.validate(df)

        overall_pass = all(r["pass"] for r in results.values())
        return {
            "status": "PASS" if overall_pass else "FAIL",
            "checks": results
        }