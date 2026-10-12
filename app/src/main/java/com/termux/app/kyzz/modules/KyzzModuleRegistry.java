package com.termux.app.kyzz.modules;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class KyzzModuleRegistry {

    public static List<KyzzModuleItem> getDefaultModules(String prefixPath) {
        List<KyzzModuleItem> list = new ArrayList<>();

        // Development
        list.add(new KyzzModuleItem("git", "Fast, scalable, distributed revision control system", "Development", "pkg", "pkg install -y git", checkBinaryInstalled(prefixPath, "git"), "latest"));
        list.add(new KyzzModuleItem("nodejs", "JavaScript runtime built on Chrome's V8 engine", "Development", "pkg", "pkg install -y nodejs", checkBinaryInstalled(prefixPath, "node"), "latest"));
        list.add(new KyzzModuleItem("python", "High-level programming language", "Development", "pkg", "pkg install -y python", checkBinaryInstalled(prefixPath, "python"), "latest"));
        list.add(new KyzzModuleItem("golang", "Go programming language compiler", "Development", "pkg", "pkg install -y golang", checkBinaryInstalled(prefixPath, "go"), "latest"));
        list.add(new KyzzModuleItem("rust", "Systems programming language that runs blazingly fast", "Development", "pkg", "pkg install -y rust", checkBinaryInstalled(prefixPath, "rustc"), "latest"));
        list.add(new KyzzModuleItem("clang", "C/C++ language compiler front-end", "Development", "pkg", "pkg install -y clang", checkBinaryInstalled(prefixPath, "clang"), "latest"));
        list.add(new KyzzModuleItem("make", "Tool which controls the generation of executables", "Development", "pkg", "pkg install -y make", checkBinaryInstalled(prefixPath, "make"), "latest"));

        // Editors
        list.add(new KyzzModuleItem("neovim", "Vim-fork focused on extensibility and usability", "Editors", "pkg", "pkg install -y neovim", checkBinaryInstalled(prefixPath, "nvim"), "latest"));
        list.add(new KyzzModuleItem("nano", "Small, friendly text editor inspired by Pico", "Editors", "pkg", "pkg install -y nano", checkBinaryInstalled(prefixPath, "nano"), "latest"));
        list.add(new KyzzModuleItem("vim", "Vim the ubiquitous text editor", "Editors", "pkg", "pkg install -y vim", checkBinaryInstalled(prefixPath, "vim"), "latest"));
        list.add(new KyzzModuleItem("micro", "Modern and intuitive terminal-based text editor", "Editors", "pkg", "pkg install -y micro", checkBinaryInstalled(prefixPath, "micro"), "latest"));

        // Network & SSH
        list.add(new KyzzModuleItem("openssh", "Secure shell remote access daemon and client", "Network & SSH", "pkg", "pkg install -y openssh", checkBinaryInstalled(prefixPath, "ssh"), "latest"));
        list.add(new KyzzModuleItem("curl", "Command line tool for transferring data with URLs", "Network & SSH", "pkg", "pkg install -y curl", checkBinaryInstalled(prefixPath, "curl"), "latest"));
        list.add(new KyzzModuleItem("wget", "Tool for retrieving files using HTTP, HTTPS, FTP", "Network & SSH", "pkg", "pkg install -y wget", checkBinaryInstalled(prefixPath, "wget"), "latest"));
        list.add(new KyzzModuleItem("rsync", "Fast, versatile, remote and local file-copying tool", "Network & SSH", "pkg", "pkg install -y rsync", checkBinaryInstalled(prefixPath, "rsync"), "latest"));

        // System Utilities
        list.add(new KyzzModuleItem("tmux", "Terminal multiplexer", "System Utilities", "pkg", "pkg install -y tmux", checkBinaryInstalled(prefixPath, "tmux"), "latest"));
        list.add(new KyzzModuleItem("htop", "Interactive process viewer and system monitor", "System Utilities", "pkg", "pkg install -y htop", checkBinaryInstalled(prefixPath, "htop"), "latest"));
        list.add(new KyzzModuleItem("tree", "Recursive directory listing command", "System Utilities", "pkg", "pkg install -y tree", checkBinaryInstalled(prefixPath, "tree"), "latest"));
        list.add(new KyzzModuleItem("jq", "Command-line JSON processor", "System Utilities", "pkg", "pkg install -y jq", checkBinaryInstalled(prefixPath, "jq"), "latest"));
        list.add(new KyzzModuleItem("unzip", "Extraction utility for archives compressed in .zip", "System Utilities", "pkg", "pkg install -y unzip", checkBinaryInstalled(prefixPath, "unzip"), "latest"));

        // Java & Android
        list.add(new KyzzModuleItem("openjdk-17", "OpenJDK 17 Java Runtime Environment", "Java & Android", "pkg", "pkg install -y openjdk-17", checkBinaryInstalled(prefixPath, "java"), "17"));

        // Node.js NPM modules
        list.add(new KyzzModuleItem("typescript", "TypeScript is language for application-scale JavaScript", "Node.js Modules", "npm", "npm install -g typescript", false, "latest"));
        list.add(new KyzzModuleItem("yarn", "Fast, reliable, and secure dependency management", "Node.js Modules", "npm", "npm install -g yarn", false, "latest"));
        list.add(new KyzzModuleItem("pnpm", "Fast, disk space efficient package manager", "Node.js Modules", "npm", "npm install -g pnpm", false, "latest"));

        // Python PIP modules
        list.add(new KyzzModuleItem("pip", "The PyPA recommended tool for installing Python packages", "Python Modules", "pkg", "pkg install -y python-pip", checkBinaryInstalled(prefixPath, "pip"), "latest"));
        list.add(new KyzzModuleItem("virtualenv", "Virtual Python Environment builder", "Python Modules", "pip", "pip install virtualenv", false, "latest"));
        list.add(new KyzzModuleItem("requests", "Python HTTP for Humans", "Python Modules", "pip", "pip install requests", false, "latest"));

        return list;
    }

    private static boolean checkBinaryInstalled(String prefixPath, String binaryName) {
        if (prefixPath == null || prefixPath.trim().isEmpty()) {
            prefixPath = "/data/data/com.termux/files/usr";
        }
        File bin = new File(prefixPath + "/bin/" + binaryName);
        return bin.exists() && bin.canExecute();
    }
}
