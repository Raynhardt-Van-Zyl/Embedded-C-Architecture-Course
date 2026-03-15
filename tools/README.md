# Course Maintenance Tools

## `course_audit.py`

Runs structural and factual consistency checks for this curriculum:

- Placeholder metadata/text detection (`your-org`, unresolved placeholders)
- Canonical chapter-title consistency (`src/SUMMARY.md` vs chapter `index.md` H1)
- Presence of standard chapter-template sections
- Hardware README references to missing directories

### Usage

```bash
python tools/course_audit.py
```
