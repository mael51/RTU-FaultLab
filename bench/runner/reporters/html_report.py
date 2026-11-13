"""Generate a synthetic HTML report."""

def write_html(campaign_id: str, results: list, path: str):
    rows = ""
    for r in results:
        status = "PASS" if r["passed"] else "FAIL"
        color = "green" if r["passed"] else "red"
        rows += f"<tr><td>{r['id']}</td><td style='color:{color}'>{status}</td><td>{r.get('response')}</td></tr>\n"
    html = f"""<!DOCTYPE html>
<html><head><meta charset='utf-8'><title>{campaign_id}</title></head>
<body><h1>{campaign_id}</h1>
<table border='1'><tr><th>Test</th><th>Status</th><th>Response</th></tr>
{rows}</table></body></html>"""
    with open(path, "w", encoding="utf-8") as f:
        f.write(html)