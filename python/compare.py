"""Run both scheduling policies and report workload metrics (standard library only)."""
import argparse, csv, io, json, subprocess
p=argparse.ArgumentParser();p.add_argument("--binary",default="build/gpusim");p.add_argument("--workload",default="examples/workload.csv");a=p.parse_args()
for policy in ("priority","backfill"):
 rows=list(csv.DictReader(io.StringIO(subprocess.check_output([a.binary,a.workload,"2","16",policy],text=True))))
 print(json.dumps(dict(policy=policy,jobs=len(rows),makespan=max((float(r["finish"]) for r in rows),default=0),mean_wait=sum(float(r["wait"]) for r in rows)/max(1,len(rows)))))
