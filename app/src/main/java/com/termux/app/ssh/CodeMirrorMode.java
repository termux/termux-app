package com.termux.app.ssh;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.HashSet;

/**
 * Enumeration mapping file extensions to CodeMirror 5 editor modes.
 *
 * Each mode defines the syntax highlighting mode name and optional CDN path
 * for loading the language-specific mode JavaScript file.
 */
public enum CodeMirrorMode {

    // Scripting languages
    PYTHON("python", "python", "mode/python/python.min.js"),
    JAVASCRIPT("javascript", "javascript", "mode/javascript/javascript.min.js"),
    SHELL("shell", "shell", "mode/shell/shell.min.js"),
    PHP("php", "php", "mode/php/php.min.js"),

    // Compiled languages (using clike mode)
    JAVA("text/x-java", "clike", "mode/clike/clike.min.js"),
    C("text/x-c", "clike", "mode/clike/clike.min.js"),
    CPP("text/x-c++src", "clike", "mode/clike/clike.min.js"),
    GO("go", "go", "mode/go/go.min.js"),

    // Markup and styling
    HTML("htmlmixed", "htmlmixed", "mode/htmlmixed/htmlmixed.min.js"),
    CSS("css", "css", "mode/css/css.min.js"),
    XML("xml", "xml", "mode/xml/xml.min.js"),
    MARKDOWN("markdown", "markdown", "mode/markdown/markdown.min.js"),

    // Data formats
    JSON("javascript", "javascript", "mode/javascript/javascript.min.js"),  // JSON uses JavaScript mode
    YAML("yaml", "yaml", "mode/yaml/yaml.min.js"),
    SQL("sql", "sql", "mode/sql/sql.min.js"),

    // Other common formats
    RUST("rust", "rust", "mode/rust/rust.min.js"),
    TYPESCRIPT("typescript", "javascript", "mode/javascript/javascript.min.js"),  // TypeScript uses JS mode base
    PERL("perl", "perl", "mode/perl/perl.min.js"),
    RUBY("ruby", "ruby", "mode/ruby/ruby.min.js"),
    LUA("lua", "lua", "mode/lua/lua.min.js"),
    DIFF("diff", "diff", "mode/diff/diff.min.js"),

    // Default/fallback
    TEXT("text/plain", null, null);

    /** The CodeMirror mode name (MIME type or mode identifier) */
    private final String modeName;

    /** The mode module name for CDN loading (may be null for built-in modes) */
    private final String moduleName;

    /** The CDN path relative to CodeMirror base URL (may be null for built-in modes) */
    private final String cdnPath;

    /** Static map for extension lookup */
    private static final Map<String, CodeMirrorMode> EXTENSION_MAP = new HashMap<>();

    /** Set of editable file extensions */
    private static final Set<String> EDITABLE_EXTENSIONS = new HashSet<>();

    static {
        // Initialize extension mappings
        EXTENSION_MAP.put("py", PYTHON);
        EXTENSION_MAP.put("pyw", PYTHON);
        EXTENSION_MAP.put("js", JAVASCRIPT);
        EXTENSION_MAP.put("mjs", JAVASCRIPT);
        EXTENSION_MAP.put("cjs", JAVASCRIPT);
        EXTENSION_MAP.put("java", JAVA);
        EXTENSION_MAP.put("c", C);
        EXTENSION_MAP.put("h", C);
        EXTENSION_MAP.put("cpp", CPP);
        EXTENSION_MAP.put("cc", CPP);
        EXTENSION_MAP.put("cxx", CPP);
        EXTENSION_MAP.put("hpp", CPP);
        EXTENSION_MAP.put("hh", CPP);
        EXTENSION_MAP.put("hxx", CPP);
        EXTENSION_MAP.put("html", HTML);
        EXTENSION_MAP.put("htm", HTML);
        EXTENSION_MAP.put("xhtml", HTML);
        EXTENSION_MAP.put("css", CSS);
        EXTENSION_MAP.put("scss", CSS);
        EXTENSION_MAP.put("less", CSS);
        EXTENSION_MAP.put("json", JSON);
        EXTENSION_MAP.put("jsonl", JSON);
        EXTENSION_MAP.put("xml", XML);
        EXTENSION_MAP.put("xsl", XML);
        EXTENSION_MAP.put("xslt", XML);
        EXTENSION_MAP.put("svg", XML);
        EXTENSION_MAP.put("sh", SHELL);
        EXTENSION_MAP.put("bash", SHELL);
        EXTENSION_MAP.put("zsh", SHELL);
        EXTENSION_MAP.put("ksh", SHELL);
        EXTENSION_MAP.put("md", MARKDOWN);
        EXTENSION_MAP.put("markdown", MARKDOWN);
        EXTENSION_MAP.put("yaml", YAML);
        EXTENSION_MAP.put("yml", YAML);
        EXTENSION_MAP.put("sql", SQL);
        EXTENSION_MAP.put("php", PHP);
        EXTENSION_MAP.put("phtml", PHP);
        EXTENSION_MAP.put("php3", PHP);
        EXTENSION_MAP.put("php4", PHP);
        EXTENSION_MAP.put("php5", PHP);
        EXTENSION_MAP.put("go", GO);
        EXTENSION_MAP.put("rs", RUST);
        EXTENSION_MAP.put("ts", TYPESCRIPT);
        EXTENSION_MAP.put("tsx", TYPESCRIPT);
        EXTENSION_MAP.put("pl", PERL);
        EXTENSION_MAP.put("pm", PERL);
        EXTENSION_MAP.put("rb", RUBY);
        EXTENSION_MAP.put("ruby", RUBY);
        EXTENSION_MAP.put("lua", LUA);
        EXTENSION_MAP.put("diff", DIFF);
        EXTENSION_MAP.put("patch", DIFF);

        // Editable extensions (all mapped extensions plus common text files)
        EDITABLE_EXTENSIONS.addAll(EXTENSION_MAP.keySet());
        EDITABLE_EXTENSIONS.add("txt");
        EDITABLE_EXTENSIONS.add("log");
        EDITABLE_EXTENSIONS.add("conf");
        EDITABLE_EXTENSIONS.add("config");
        EDITABLE_EXTENSIONS.add("ini");
        EDITABLE_EXTENSIONS.add("properties");
        EDITABLE_EXTENSIONS.add("cfg");
        EDITABLE_EXTENSIONS.add("rc");
        EDITABLE_EXTENSIONS.add("env");
        EDITABLE_EXTENSIONS.add("gitignore");
        EDITABLE_EXTENSIONS.add("dockerignore");
        EDITABLE_EXTENSIONS.add("makefile");
        EDITABLE_EXTENSIONS.add("rakefile");
        EDITABLE_EXTENSIONS.add("gemfile");
        EDITABLE_EXTENSIONS.add("podfile");
        EDITABLE_EXTENSIONS.add("vagrantfile");
        EDITABLE_EXTENSIONS.add("license");
        EDITABLE_EXTENSIONS.add("readme");
        EDITABLE_EXTENSIONS.add("changelog");
        EDITABLE_EXTENSIONS.add("authors");
        EDITABLE_EXTENSIONS.add("todo");
        EDITABLE_EXTENSIONS.add("csv");
        EDITABLE_EXTENSIONS.add("tsv");
    }

    /**
     * Create a CodeMirrorMode.
     *
     * @param modeName The CodeMirror mode/MIME type name
     * @param moduleName The mode module name for CDN loading (null for built-in)
     * @param cdnPath The CDN path relative to CodeMirror base URL (null for built-in)
     */
    CodeMirrorMode(String modeName, String moduleName, String cdnPath) {
        this.modeName = modeName;
        this.moduleName = moduleName;
        this.cdnPath = cdnPath;
    }

    /**
     * Get the CodeMirror mode name.
     *
     * @return Mode name (MIME type or identifier)
     */
    public String getModeName() {
        return modeName;
    }

    /**
     * Get the mode module name.
     *
     * @return Module name (null for built-in modes)
     */
    public String getModuleName() {
        return moduleName;
    }

    /**
     * Get the CDN path for this mode.
     *
     * @return CDN path relative to base URL (null for built-in modes)
     */
    public String getCdnPath() {
        return cdnPath;
    }

    /**
     * Check if this mode requires external mode loading.
     *
     * @return true if CDN path is defined
     */
    public boolean requiresExternalMode() {
        return cdnPath != null;
    }

    /**
     * Get the CodeMirror mode for a file extension.
     *
     * @param extension File extension (without leading dot, case-insensitive)
     * @return Corresponding CodeMirrorMode, or TEXT if not found
     */
    public static CodeMirrorMode getModeFromExtension(String extension) {
        if (extension == null || extension.isEmpty()) {
            return TEXT;
        }
        String lowerExt = extension.toLowerCase().trim();
        CodeMirrorMode mode = EXTENSION_MAP.get(lowerExt);
        return mode != null ? mode : TEXT;
    }

    /**
     * Get the CodeMirror mode for a filename.
     *
     * @param filename Full filename with extension
     * @return Corresponding CodeMirrorMode, or TEXT if extension not recognized
     */
    public static CodeMirrorMode getModeFromFilename(String filename) {
        if (filename == null || filename.isEmpty()) {
            return TEXT;
        }

        // Check for special files without extensions (case-insensitive)
        String lowerName = filename.toLowerCase().trim();
        if (lowerName.equals("makefile") || lowerName.equals("rakefile") ||
            lowerName.equals("gemfile") || lowerName.equals("podfile") ||
            lowerName.equals("vagrantfile") || lowerName.equals("dockerfile")) {
            return SHELL;  // These are essentially shell-like syntax
        }
        if (lowerName.equals("license") || lowerName.equals("readme") ||
            lowerName.equals("changelog") || lowerName.equals("authors") ||
            lowerName.equals("todo") || lowerName.equals("contributing")) {
            return MARKDOWN;
        }
        if (lowerName.startsWith(".git") || lowerName.equals(".env") ||
            lowerName.equals(".dockerignore") || lowerName.startsWith(".bash") ||
            lowerName.startsWith(".zsh")) {
            return SHELL;
        }

        // Extract extension
        int dotIndex = filename.lastIndexOf('.');
        if (dotIndex <= 0 || dotIndex == filename.length() - 1) {
            return TEXT;
        }
        String extension = filename.substring(dotIndex + 1);
        return getModeFromExtension(extension);
    }

    /**
     * Check if a file is editable based on its extension.
     *
     * @param filename Full filename
     * @return true if the file appears to be a text file that can be edited
     */
    public static boolean isEditableFile(String filename) {
        if (filename == null || filename.isEmpty()) {
            return false;
        }

        String lowerName = filename.toLowerCase().trim();

        // Check for special files without extensions
        if (lowerName.equals("makefile") || lowerName.equals("rakefile") ||
            lowerName.equals("gemfile") || lowerName.equals("podfile") ||
            lowerName.equals("vagrantfile") || lowerName.equals("dockerfile") ||
            lowerName.equals("license") || lowerName.equals("readme") ||
            lowerName.equals("changelog") || lowerName.equals("authors") ||
            lowerName.equals("todo") || lowerName.equals("contributing")) {
            return true;
        }

        // Check dotfiles
        if (lowerName.startsWith(".") && !lowerName.contains("/")) {
            // Most dotfiles are editable
            return true;
        }

        // Extract extension
        int dotIndex = filename.lastIndexOf('.');
        if (dotIndex <= 0 || dotIndex == filename.length() - 1) {
            // No extension - check special cases
            return false;
        }

        String extension = filename.substring(dotIndex + 1).toLowerCase().trim();
        return EDITABLE_EXTENSIONS.contains(extension);
    }

    /**
     * Get all mode CDN paths that need to be loaded for the given modes.
     *
     * @param modes Array of CodeMirrorModes to check
     * @return Set of unique CDN paths (excluding nulls)
     */
    public static Set<String> getRequiredCdnPaths(CodeMirrorMode... modes) {
        Set<String> paths = new HashSet<>();
        for (CodeMirrorMode mode : modes) {
            if (mode != null && mode.requiresExternalMode()) {
                paths.add(mode.cdnPath);
            }
        }
        return paths;
    }

    /**
     * Get all supported file extensions.
     *
     * @return Set of all known editable extensions
     */
    public static Set<String> getAllSupportedExtensions() {
        return new HashSet<>(EDITABLE_EXTENSIONS);
    }
}