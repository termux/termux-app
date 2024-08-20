package com.termux.x11.controller.core;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public abstract class MSLink {
    public static final byte SW_SHOWNORMAL = 1;
    public static final byte SW_SHOWMAXIMIZED = 3;
    public static final byte SW_SHOWMINNOACTIVE = 7;
    private static final int HasLinkTargetIDList = 1<<0;
    private static final int HasArguments = 1<<5;
    private static final int HasIconLocation = 1<<6;
    private static final int ForceNoLinkInfo = 1<<8;

    public static final class Options {
        public String targetPath;
        public String cmdArgs;
        public String iconLocation;
        public int iconIndex;
        public int fileSize;
        public int showCommand = SW_SHOWNORMAL;
    }

    private static int charToHexDigit(char chr) {
        return chr >= 'A' ? chr - 'A' + 10 : chr - '0';
    }

    private static byte twoCharsToByte(char chr1, char chr2) {
        return (byte)(charToHexDigit(Character.toUpperCase(chr1)) * 16 + charToHexDigit(Character.toUpperCase(chr2)));
    }

    private static byte[] convertCLSIDtoDATA(String str) {
        return new byte[]{
            twoCharsToByte(str.charAt(6), str.charAt(7)),
            twoCharsToByte(str.charAt(4), str.charAt(5)),
            twoCharsToByte(str.charAt(2), str.charAt(3)),
            twoCharsToByte(str.charAt(0), str.charAt(1)),
            twoCharsToByte(str.charAt(11), str.charAt(12)),
            twoCharsToByte(str.charAt(9), str.charAt(10)),
            twoCharsToByte(str.charAt(16), str.charAt(17)),
            twoCharsToByte(str.charAt(14), str.charAt(15)),
            twoCharsToByte(str.charAt(19), str.charAt(20)),
            twoCharsToByte(str.charAt(21), str.charAt(22)),
            twoCharsToByte(str.charAt(24), str.charAt(25)),
            twoCharsToByte(str.charAt(26), str.charAt(27)),
            twoCharsToByte(str.charAt(28), str.charAt(29)),
            twoCharsToByte(str.charAt(30), str.charAt(31)),
            twoCharsToByte(str.charAt(32), str.charAt(33)),
            twoCharsToByte(str.charAt(34), str.charAt(35))
        };
    }

    private static byte[] stringToByteArray(String str) {
        byte[] bytes = new byte[str.length()];
        for (int i = 0; i < bytes.length; i++) bytes[i] = (byte)str.charAt(i);
        return bytes;
    }

    private static byte[] intToByteArray(int value) {
        return ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(value).array();
    }

    private static byte[] stringSizePaddedToByteArray(String str) {
        ByteBuffer buffer = ByteBuffer.allocate(str.length() + 2).order(ByteOrder.LITTLE_ENDIAN);
        buffer.putShort((short)str.length());
        for (int i = 0; i < str.length(); i++) buffer.put((byte)str.charAt(i));
        return buffer.array();
    }

    private static byte[] generateIDLIST(byte[] bytes) {
        ByteBuffer buffer = ByteBuffer.allocate(2).order(ByteOrder.LITTLE_ENDIAN).putShort((short)(bytes.length + 2));
        return ArrayUtils.concat(buffer.array(), bytes);
    }

    public static void createFile(String targetPath, File outputFile) {
        Options options = new Options();
        options.targetPath = targetPath;
        createFile(options, outputFile);
    }

    public static void createFile(Options options, File outputFile) {
        byte[] HeaderSize = new byte[]{0x4c, 0x00, 0x00, 0x00};
        byte[] LinkCLSID = convertCLSIDtoDATA("00021401-0000-0000-c000-000000000046");

        int linkFlags = HasLinkTargetIDList | ForceNoLinkInfo;
        if (options.cmdArgs != null && !options.cmdArgs.isEmpty()) linkFlags |= HasArguments;
        if (options.iconLocation != null && !options.iconLocation.isEmpty()) linkFlags |= HasIconLocation;

        byte[] LinkFlags = intToByteArray(linkFlags);

        byte[] FileAttributes, prefixOfTarget;
        options.targetPath = options.targetPath.replaceAll("/+", "\\\\");
        if (options.targetPath.endsWith("\\")) {
            FileAttributes = new byte[]{0x10, 0x00, 0x00, 0x00};
            prefixOfTarget = new byte[]{0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            options.targetPath = options.targetPath.replaceAll("\\\\+$", "");
        }
        else {
            FileAttributes = new byte[]{0x20, 0x00, 0x00, 0x00};
            prefixOfTarget = new byte[]{0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        }

        byte[] CreationTime, AccessTime, WriteTime;
        CreationTime = AccessTime = WriteTime = new byte[]{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

        byte[] FileSize = intToByteArray(options.fileSize);
        byte[] IconIndex = intToByteArray(options.iconIndex);
        byte[] ShowCommand = intToByteArray(options.showCommand);
        byte[] Hotkey = new byte[]{0x00, 0x00};
        byte[] Reserved1 = new byte[]{0x00, 0x00};
        byte[] Reserved2 = new byte[]{0x00, 0x00, 0x00, 0x00};
        byte[] Reserved3 = new byte[]{0x00, 0x00, 0x00, 0x00};

        byte[] CLSIDComputer = convertCLSIDtoDATA("20d04fe0-3aea-1069-a2d8-08002b30309d");
        byte[] CLSIDNetwork = convertCLSIDtoDATA("208d2c60-3aea-1069-a2d7-08002b30309d");

        byte[] itemData, prefixRoot, targetRoot, targetLeaf;
        if (options.targetPath.startsWith("\\")) {
            prefixRoot = new byte[]{(byte)0xc3, 0x01, (byte)0x81};
            targetRoot = stringToByteArray(options.targetPath);
            targetLeaf = !options.targetPath.endsWith("\\") ? stringToByteArray(options.targetPath.substring(options.targetPath.lastIndexOf("\\") + 1)) : null;
            itemData = ArrayUtils.concat(new byte[]{0x1f, 0x58}, CLSIDNetwork);
        }
        else {
            prefixRoot = new byte[]{0x2f};
            int index = options.targetPath.indexOf("\\");
            targetRoot = stringToByteArray(options.targetPath.substring(0, index+1));
            targetLeaf = stringToByteArray(options.targetPath.substring(index+1));
            itemData = ArrayUtils.concat(new byte[]{0x1f, 0x50}, CLSIDComputer);
        }

        targetRoot = ArrayUtils.concat(targetRoot, new byte[21]);

        byte[] endOfString = new byte[]{0x00};
        byte[] IDListItems = ArrayUtils.concat(generateIDLIST(itemData), generateIDLIST(ArrayUtils.concat(prefixRoot, targetRoot, endOfString)));
        if (targetLeaf != null) IDListItems = ArrayUtils.concat(IDListItems, generateIDLIST(ArrayUtils.concat(prefixOfTarget, targetLeaf, endOfString)));
        byte[] IDList = generateIDLIST(IDListItems);

        byte[] TerminalID = new byte[]{0x00, 0x00};

        byte[] StringData = new byte[0];
        if ((linkFlags & HasArguments) != 0) StringData = ArrayUtils.concat(StringData, stringSizePaddedToByteArray(options.cmdArgs));
        if ((linkFlags & HasIconLocation) != 0) StringData = ArrayUtils.concat(StringData, stringSizePaddedToByteArray(options.iconLocation));

        try (FileOutputStream os = new FileOutputStream(outputFile)) {
            os.write(HeaderSize);
            os.write(LinkCLSID);
            os.write(LinkFlags);
            os.write(FileAttributes);
            os.write(CreationTime);
            os.write(AccessTime);
            os.write(WriteTime);
            os.write(FileSize);
            os.write(IconIndex);
            os.write(ShowCommand);
            os.write(Hotkey);
            os.write(Reserved1);
            os.write(Reserved2);
            os.write(Reserved3);
            os.write(IDList);
            os.write(TerminalID);

            if (StringData.length > 0) os.write(StringData);
        }
        catch (IOException e) {
            e.printStackTrace();
        }
    }
}
