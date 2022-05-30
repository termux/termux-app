package com.termux.shared.android.resource;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.data.DataUtils;
import com.termux.shared.logger.Logger;

public class ResourceUtils {

    public static final String RES_TYPE_COLOR = "color";
    public static final String RES_TYPE_DRAWABLE = "drawable";
    public static final String RES_TYPE_ID = "id";
    public static final String RES_TYPE_LAYOUT = "layout";
    public static final String RES_TYPE_STRING = "string";
    public static final String RES_TYPE_STYLE = "style";


    private static final String LOG_TAG = "ResourceUtils";


    /** Wrapper for {@link #getResourceId(Context, String, String, String, boolean)} without {@code defPackage}. */
    @Nullable
    public static Integer getResourceId(@NonNull Context context, String name,
                                        @Nullable String defType,
                                        boolean logErrorMessage) {
        return getResourceId(context, name, defType, null, logErrorMessage);
    }

    /**
     * Get resource identifier for the given resource name. A fully qualified resource name is of
     * the form "package:type/entry".  The first two components (package and type) are optional if
     * defType and defPackage, respectively, are specified here.
     *
     * @param context The {@link Context} for operations.
     * @param name The name of the desired resource.
     * @param defType Optional default resource type to find, if "type/" is not included in the name.
     *                Can be null to require an explicit type.
     * @param defPackage Optional default package to find, if "package:" is not included in the name.
     *                   Can be null to require an explicit package.
     * @param logErrorMessage If an error message should be logged if failed to find resource.
     * @return Returns the resource identifier if found. Otherwise {@code null} if an exception was
     * raised or resource was not found.
     */
    @Nullable
    public static Integer getResourceId(@NonNull Context context, String name,
                                        @Nullable String defType, @Nullable String defPackage,
                                        boolean logErrorMessage) {
        if (DataUtils.isNullOrEmpty(name)) return null;

        Integer resourceId = null;
        try {
            resourceId = context.getResources().getIdentifier(name, defType, defPackage);
            if (resourceId == 0) resourceId = null;
        } catch (Exception e) {
            // Ignore
        }

        if (resourceId == null && logErrorMessage) {
            Logger.logError(LOG_TAG, "Resource id not found. name: \"" + name + "\", type: \"" + defType+ "\", package: \"" + defPackage + "\", component \"" + context.getClass().getName() + "\"");
        }

        return resourceId;
    }



    /**
     * Get resource identifier for the given {@link #RES_TYPE_COLOR} resource name.
     *
     * This is a wrapper for {@link #getResourceId(Context, String, String, String, boolean)}.
     */
    @Nullable
    public static Integer getColorResourceId(@NonNull Context context, String name,
                                             @Nullable String defPackage, boolean logErrorMessage) {
        return getResourceId(context, name, RES_TYPE_COLOR, defPackage, logErrorMessage);
    }

    /**
     * Get resource identifier for the given {@link #RES_TYPE_DRAWABLE} resource name.
     *
     * This is a wrapper for {@link #getResourceId(Context, String, String, String, boolean)}.
     */
    @Nullable
    public static Integer getDrawableResourceId(@NonNull Context context, String name,
                                                @Nullable String defPackage, boolean logErrorMessage) {
        return getResourceId(context, name, RES_TYPE_DRAWABLE, defPackage, logErrorMessage);
    }

    /**
     * Get resource identifier for the given {@link #RES_TYPE_ID} resource name.
     *
     * This is a wrapper for {@link #getResourceId(Context, String, String, String, boolean)}.
     */
    @Nullable
    public static Integer getIdResourceId(@NonNull Context context, String name,
                                          @Nullable String defPackage, boolean logErrorMessage) {
        return getResourceId(context, name, RES_TYPE_ID, defPackage, logErrorMessage);
    }

    /**
     * Get resource identifier for the given {@link #RES_TYPE_LAYOUT} resource name.
     *
     * This is a wrapper for {@link #getResourceId(Context, String, String, String, boolean)}.
     */
    @Nullable
    public static Integer getLayoutResourceId(@NonNull Context context, String name,
                                              @Nullable String defPackage, boolean logErrorMessage) {
        return getResourceId(context, name, RES_TYPE_LAYOUT, defPackage, logErrorMessage);
    }

    /**
     * Get resource identifier for the given {@link #RES_TYPE_STRING} resource name.
     *
     * This is a wrapper for {@link #getResourceId(Context, String, String, String, boolean)}.
     */
    @Nullable
    public static Integer getStringResourceId(@NonNull Context context, String name,
                                              @Nullable String defPackage, boolean logErrorMessage) {
        return getResourceId(context, name, RES_TYPE_STRING, defPackage, logErrorMessage);
    }

    /**
     * Get resource identifier for the given {@link #RES_TYPE_STYLE} resource name.
     *
     * This is a wrapper for {@link #getResourceId(Context, String, String, String, boolean)}.
     */
    @Nullable
    public static Integer getStyleResourceId(@NonNull Context context, String name,
                                             @Nullable String defPackage, boolean logErrorMessage) {
        return getResourceId(context, name, RES_TYPE_STYLE, defPackage, logErrorMessage);
    }

}
