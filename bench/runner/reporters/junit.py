"""Generate JUnit XML report."""

import xml.etree.ElementTree as ET


def write_junit(results: list, path: str, suite_name: str = "RTU-FaultLab"):
    testsuite = ET.Element("testsuite", name=suite_name, tests=str(len(results)))
    failures = sum(0 if r["passed"] else 1 for r in results)
    testsuite.set("failures", str(failures))
    testsuite.set("errors", "0")
    for r in results:
        tc = ET.SubElement(testsuite, "testcase", name=r["id"], classname=suite_name)
        if not r["passed"]:
            fail = ET.SubElement(tc, "failure", message="assertion failed")
            fail.text = f"response={r.get('response')} timed_out={r.get('timed_out')}"
    tree = ET.ElementTree(testsuite)
    ET.indent(tree, space="  ")
    tree.write(path, encoding="utf-8", xml_declaration=True)