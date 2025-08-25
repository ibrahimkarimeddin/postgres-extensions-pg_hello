# Documentation Index

Complete documentation for the PostgreSQL `pg_hello` extension - your comprehensive guide to understanding PostgreSQL extension development.

## 📚 Documentation Overview

This documentation set covers everything from basic concepts to advanced PostgreSQL extension development techniques. Whether you're new to PostgreSQL extensions or looking to deepen your understanding, these guides will help you master the technology.

## 🗂️ Document Structure

### 📖 [Main README](../README.md)
**Start here!** Overview of the extension, quick start guide, and basic usage examples.

**What you'll learn:**
- Extension overview and features
- Quick installation and setup
- Basic usage examples
- Project structure

**Good for:** First-time users, getting started quickly

---

### 🔧 [Makefile Documentation](MAKEFILE.md)
Complete guide to understanding the build system and Make automation.

**What you'll learn:**
- What is Make and why it's used
- Line-by-line Makefile explanation
- PGXS (PostgreSQL Extension Building Infrastructure)
- Build commands and customization
- Troubleshooting build issues

**Good for:** Understanding the build process, customizing builds, debugging build problems

---

### 📁 [File Types Guide](FILE_TYPES.md)
Comprehensive explanation of all file types in PostgreSQL extension development.

**What you'll learn:**
- `.c` files (C source code)
- `.o` files (object files) 
- `.so` files (shared libraries)
- `.control` files (extension metadata)
- `.sql` files (SQL definitions)
- Build artifacts and temporary files
- File relationships and dependencies

**Good for:** Understanding what each file does, debugging file-related issues, project organization

---

### 💻 [C Code Structure](C_CODE.md)
Deep dive into the C implementation and PostgreSQL's C API.

**What you'll learn:**
- PostgreSQL C API fundamentals
- Function anatomy and structure
- Data type conversion between PostgreSQL and C
- Memory management in PostgreSQL
- Error handling patterns
- GUC (configuration) system
- SPI (Server Programming Interface)
- Best practices and common patterns

**Good for:** C developers, understanding the implementation, writing your own functions

---

### 🏗️ [Extension System](EXTENSION_SYSTEM.md)
How PostgreSQL extensions work internally, from installation to runtime.

**What you'll learn:**
- Extension architecture and components
- Extension lifecycle (install, upgrade, remove)
- Version management and upgrades
- Dependency management
- Security considerations
- System catalogs and metadata
- Advanced extension features

**Good for:** Understanding extension internals, managing extensions in production, advanced development

---

### 🔨 [Build Process](BUILD_PROCESS.md)
Detailed guide to the complete build and installation process.

**What you'll learn:**
- Prerequisites and environment setup
- Step-by-step build process
- Compilation and linking phases
- Installation procedures
- Cross-platform considerations
- Testing and validation
- Advanced build topics

**Good for:** Setting up development environment, understanding compilation, deployment, CI/CD

---

### ⚡ [How It Works](HOW_IT_WORKS.md)
Deep dive into how PostgreSQL discovers, loads, and executes your extension code.

**What you'll learn:**
- Extension loading process step-by-step
- Function registration and symbol resolution
- Memory layout and dynamic loading
- Runtime execution flow
- Internal PostgreSQL structures
- Debugging and tracing techniques

**Good for:** Understanding PostgreSQL internals, debugging extension issues, curious developers

## 🎯 Learning Paths

### 🌱 **Beginner Path**
New to PostgreSQL extensions? Start here:

1. **[Main README](../README.md)** - Get overview and install the extension
2. **[File Types Guide](FILE_TYPES.md)** - Understand what each file does
3. **[Makefile Documentation](MAKEFILE.md)** - Learn how building works
4. **[Build Process](BUILD_PROCESS.md)** - Set up your development environment

### 🔬 **Developer Path**
Want to modify or create extensions? Follow this path:

1. **[C Code Structure](C_CODE.md)** - Master the C API
2. **[Extension System](EXTENSION_SYSTEM.md)** - Understand extension internals  
3. **[Build Process](BUILD_PROCESS.md)** - Advanced build and deployment
4. **[File Types Guide](FILE_TYPES.md)** - Deep dive into file relationships

### 🚀 **Advanced Path**
Ready for production deployment and advanced features?

1. **[Extension System](EXTENSION_SYSTEM.md)** - Security, dependencies, versions
2. **[Build Process](BUILD_PROCESS.md)** - Cross-platform, packaging, CI/CD
3. **[C Code Structure](C_CODE.md)** - Advanced patterns, performance, debugging

### 🔧 **Troubleshooting Path**
Having issues? Check these in order:

1. **[Build Process](BUILD_PROCESS.md)** - Build and installation problems
2. **[Makefile Documentation](MAKEFILE.md)** - Build system issues
3. **[File Types Guide](FILE_TYPES.md)** - File permission and location issues
4. **[Extension System](EXTENSION_SYSTEM.md)** - Runtime and loading issues

## 🗝️ Key Concepts Cross-Reference

### **Build System**
- **[Makefile Documentation](MAKEFILE.md)** - Complete build system guide
- **[Build Process](BUILD_PROCESS.md)** - Step-by-step compilation
- **[File Types Guide](FILE_TYPES.md)** - Object files and shared libraries

### **C Programming**
- **[C Code Structure](C_CODE.md)** - Complete C API reference
- **[File Types Guide](FILE_TYPES.md)** - Source and compiled files
- **[Build Process](BUILD_PROCESS.md)** - Compilation process

### **Extension Management**
- **[Extension System](EXTENSION_SYSTEM.md)** - Complete extension lifecycle
- **[File Types Guide](FILE_TYPES.md)** - Control and SQL files
- **[Main README](../README.md)** - Basic usage

### **Configuration and Setup**
- **[Build Process](BUILD_PROCESS.md)** - Environment setup
- **[C Code Structure](C_CODE.md)** - GUC configuration system
- **[Extension System](EXTENSION_SYSTEM.md)** - Extension configuration

## 🔍 Quick Reference

### Common Commands
```bash
# Build and install
make clean && make && sudo make install

# Create extension in database
psql -c "CREATE EXTENSION pg_hello;"

# Test extension
psql -c "SELECT pg_hello('World');"

# Run tests
make installcheck
```

### Important Files
- **`src/pg_hello.c`** - Main C implementation
- **`pg_hello.control`** - Extension metadata
- **`sql/pg_hello--1.0.sql`** - SQL function definitions
- **`Makefile`** - Build configuration

### Key Directories
- **`$(pg_config --sharedir)/extension/`** - Extension files
- **`$(pg_config --libdir)/`** - Shared libraries
- **`src/`** - Source code
- **`docs/`** - Documentation

## 💡 Tips for Using This Documentation

### **Reading Order**
- **Linear**: Read documents in the order listed above
- **Reference**: Jump to specific topics as needed
- **Problem-Solving**: Use troubleshooting sections first

### **Code Examples**
- All code examples are tested and working
- Copy-paste friendly (remove line numbers if present)
- Platform-specific notes are clearly marked

### **Cross-References**
- Documents reference each other extensively
- Follow links for deeper understanding
- Use the index to find specific topics

### **Updates**
- Documentation stays synchronized with code
- Version-specific information is clearly marked
- Check timestamps for recent changes

## 🤝 Contributing to Documentation

### Reporting Issues
- Unclear explanations
- Missing topics
- Outdated information
- Broken examples

### Suggesting Improvements
- Additional examples
- Better explanations
- New topics
- Cross-references

## 📚 External Resources

### PostgreSQL Documentation
- [Extension Building Infrastructure](https://www.postgresql.org/docs/current/extend-pgxs.html)
- [C Language Functions](https://www.postgresql.org/docs/current/xfunc-c.html)
- [Server Programming Interface](https://www.postgresql.org/docs/current/spi.html)

### Development Tools
- [GNU Make Manual](https://www.gnu.org/software/make/manual/)
- [GCC Documentation](https://gcc.gnu.org/onlinedocs/)
- [PostgreSQL Source Code](https://git.postgresql.org/gitweb/?p=postgresql.git)

### Community Resources
- [PostgreSQL Extension Network (PGXN)](https://pgxn.org/)
- [PostgreSQL Wiki](https://wiki.postgresql.org/)
- [PostgreSQL Mailing Lists](https://www.postgresql.org/list/)

---

**Happy Learning!** 🎉

This documentation represents a complete guide to PostgreSQL extension development. Take your time, experiment with the code, and don't hesitate to dive deep into the topics that interest you most.
