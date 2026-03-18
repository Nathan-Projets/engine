# Documentation Guide

This project uses [Doxygen](https://www.doxygen.nl/) to generate API documentation from source code comments.

## Building Documentation

### Prerequisites
- **Doxygen** must be installed and available in your PATH
  - Windows: Download from [doxygen.nl](https://www.doxygen.nl/download.html)
  - Or install via package manager (e.g., `choco install doxygen` on Windows with Chocolatey)

### Generate Documentation

#### Option 1: Using CMake (Recommended)
```bash
# From the project root directory after running CMake
cmake --build build -t documentation
```

This will generate HTML documentation in `docs/html/index.html`.

#### Option 2: Direct Doxygen Execution
```bash
# From the project root directory
doxygen Doxyfile
```

### View Documentation

After generation, open the documentation in a web browser:
```bash
docs/html/index.html
```

## Documentation Structure

The generated documentation is organized directly by source symbols:

- Namespaces
- Classes/Structs
- Files
- Inheritance and collaboration graphs (when Graphviz is enabled)

Use this guide for tooling workflow only. For architecture and gameplay-system details, use `README.md` and the generated API pages.

## Comment Conventions

Use these styles for best output quality:

- Public APIs: `/** ... */` blocks with `@brief`, `@param`, `@return` where relevant
- Short member notes: `///` one-line comments
- Group related methods with Doxygen groups:
  - `/// @name Group Name`
  - `/// @{` and `/// @}`
- Cross-reference important symbols with `@see`

Example:

```cpp
/**
 * @brief Load a resource asynchronously.
 * @param path Relative or absolute asset path.
 * @return Type-safe resource handle.
 */
Handle<Mesh> Load(const std::string &path);
```

## Building Documentation with Graphs (Advanced)

To enable class/collaboration diagrams, ensure **Graphviz** is installed:

1. Install [Graphviz](https://graphviz.org/download/)
2. Uncomment `HAVE_DOT = YES` in the Doxyfile
3. Rebuild documentation

## CI/CD Integration

To add documentation generation to CI/CD pipelines, add this step:

```yaml
# Example GitHub Actions
- name: Generate Doxygen Documentation
  run: |
    sudo apt-get install -y doxygen  # Linux
    cmake --build build -t documentation
```

## Troubleshooting

**"Doxygen not found"** error during CMake:
- Ensure Doxygen is installed and in PATH
- On Windows, add Doxygen to Path or set `CMAKE_PROGRAM_PATH`

**No documentation target appears**:
- Verify `Doxyfile` exists in project root
- Check CMake output for Doxygen detection status

**Documentation looks incomplete**:
- Ensure comments follow Doxygen format (start with `///` or `/**`)
- Check Doxyfile `WARN_IF_UNDOCUMENTED` setting
- Review warnings in Doxygen output

**Graphs are missing**:
- Install Graphviz
- Set `HAVE_DOT = YES` in `Doxyfile`
- Regenerate docs

## References

- [Doxygen Manual](https://www.doxygen.nl/manual/index.html)
- [Doxygen Command Reference](https://www.doxygen.nl/manual/commands.html)
- [Markdown in Doxygen](https://www.doxygen.nl/manual/markdown.html)

---

*For questions about architecture concepts, see the generated documentation at `docs/html/index.html`.*
