package com.termux.floatball.menu;

import android.content.Context;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.ViewGroup;

import com.termux.floatball.runner.ICarrier;
import com.termux.floatball.runner.ScrollRunner;
import com.termux.floatball.utils.DensityUtil;


/**
 * menu layout
 *
 */
public class MenuLayout extends ViewGroup implements ICarrier {
    private int mChildSize;
    private int mChildPadding = 5;
    private float mFromDegrees;
    private float mToDegrees;
    private static int MIN_RADIUS;
    private int mRadius;// distance between center and menu item
    private boolean mExpanded = false;
    private boolean isMoving = false;
    private int position = FloatMenu.LEFT_TOP;
    private int centerX = 0;
    private int centerY = 0;
    private ScrollRunner mRunner;

    public void computeCenterXY(int position) {
        final int size = getLayoutSize();
        int width = size;
        int height = size;
        switch (position) {
            case FloatMenu.LEFT_TOP://left top
                centerX = width / 2 - getRadiusAndPadding();
                centerY = height / 2 - getRadiusAndPadding();
                break;
            case FloatMenu.LEFT_CENTER://left middle
                centerX = width / 2 - getRadiusAndPadding();
                centerY = height / 2;
                break;
            case FloatMenu.LEFT_BOTTOM://left bottom
                centerX = width / 2 - getRadiusAndPadding();
                centerY = height / 2 + getRadiusAndPadding();
                break;
            case FloatMenu.CENTER_TOP://top middle
                centerX = width / 2;
                centerY = height / 2 - getRadiusAndPadding();
                break;
            case FloatMenu.CENTER_BOTTOM://bottom middle
                centerX = width / 2;
                centerY = height / 2 + getRadiusAndPadding();
                break;
            case FloatMenu.RIGHT_TOP://right top
                centerX = width / 2 + getRadiusAndPadding();
                centerY = height / 2 - getRadiusAndPadding();
                break;
            case FloatMenu.RIGHT_CENTER://right middle
                centerX = width / 2 + getRadiusAndPadding();
                centerY = height / 2;
                break;
            case FloatMenu.RIGHT_BOTTOM://right bottom
                centerX = width / 2 + getRadiusAndPadding();
                centerY = height / 2 + getRadiusAndPadding();
                break;

            case FloatMenu.CENTER:
                centerX = width / 2;
                centerY = width / 2;
                break;
        }
    }

    private int getRadiusAndPadding() {
        return mRadius + (mChildPadding * 2);
    }

    public MenuLayout(Context context) {
        this(context, null);
    }

    public MenuLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        MIN_RADIUS = DensityUtil.dip2px(context, 40);
        mRunner = new ScrollRunner(this);
        setChildrenDrawingOrderEnabled(true);
    }

    /**
     * compute radius
     */
    private static int computeRadius(final float arcDegrees, final int childCount, final int childSize, final int childPadding, final int minRadius) {
        if (childCount < 2) {
            return minRadius;
        }
//        final float perDegrees = arcDegrees / (childCount - 1);
        final float perDegrees = arcDegrees == 360 ? (arcDegrees) / (childCount) : (arcDegrees) / (childCount - 1);
        final float perHalfDegrees = perDegrees / 2;
        final int perSize = childSize + childPadding;
        final int radius = (int) ((perSize / 2) / Math.sin(Math.toRadians(perHalfDegrees)));
        return Math.max(radius, minRadius);
    }

    /**
     * compute item show range
     */
    private static Rect computeChildFrame(final int centerX, final int centerY, final int radius, final float degrees, final int size) {
        //position of menu center
        final double childCenterX = centerX + radius * Math.cos(Math.toRadians(degrees));
        final double childCenterY = centerY + radius * Math.sin(Math.toRadians(degrees));
        //menu's postion lt, rt,lb, lr
        return new Rect((int) (childCenterX - size / 2),
                (int) (childCenterY - size / 2), (int) (childCenterX + size / 2), (int) (childCenterY + size / 2));
    }

    /**
     * size of menu item
     */
    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int size = getLayoutSize();
        setMeasuredDimension(size, size);
        final int count = getChildCount();
        for (int i = 0; i < count; i++) {
            getChildAt(i).measure(MeasureSpec.makeMeasureSpec(mChildSize, MeasureSpec.EXACTLY),
                    MeasureSpec.makeMeasureSpec(mChildSize, MeasureSpec.EXACTLY));
        }
    }

    private int getLayoutSize() {
        mRadius = computeRadius(Math.abs(mToDegrees - mFromDegrees), getChildCount(),
                mChildSize, mChildPadding, MIN_RADIUS);
        int layoutPadding = 10;
        return mRadius * 2 + mChildSize + mChildPadding + layoutPadding * 2;
    }

    /**
     * position of menu item
     */
    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        if (isMoving) return;
        computeCenterXY(position);
        final int radius = 0;
        layoutItem(radius);
    }

    @Override
    protected int getChildDrawingOrder(int childCount, int i) {
        //When the hover ball is on the right side, make its menus in the same order from top to bottom as when it is on the left side.
        if (!isLeft()) {
            return childCount - i - 1;
        }
        return i;
    }

    private boolean isLeft() {
        int corner = (int) (mFromDegrees / 90);
        return corner == 0 || corner == 3 ? true : false;
    }

    private void layoutItem(int radius) {
        final int childCount = getChildCount();
//        final float perDegrees =Math.abs (mToDegrees - mFromDegrees) / (childCount - 1);
        float perDegrees;
        float degrees = mFromDegrees;
        float arcDegrees = Math.abs(mToDegrees - mFromDegrees);
        if (childCount == 1) {
            perDegrees = arcDegrees / (childCount + 1);
            degrees += perDegrees;
        } else if (childCount == 2) {
            if (arcDegrees == 90) {
                perDegrees = arcDegrees / (childCount - 1);
            } else {
                perDegrees = arcDegrees / (childCount + 1);
                degrees += perDegrees;
            }
        } else {
            perDegrees = arcDegrees == 360 ? arcDegrees / (childCount) : arcDegrees / (childCount - 1);
        }
        for (int i = 0; i < childCount; i++) {
            int index = getChildDrawingOrder(childCount, i);
            Rect frame = computeChildFrame(centerX, centerY, radius, degrees, mChildSize);
            degrees += perDegrees;
            getChildAt(index).layout(frame.left, frame.top, frame.right, frame.bottom);
        }
    }

    @Override
    public void requestLayout() {
        if (!isMoving) {
            super.requestLayout();
        }
    }

    /**
     * Toggle center button expansion and contraction
     */
    public void switchState(int position, int duration) {
        this.position = position;
        mExpanded = !mExpanded;
        isMoving = true;
        mRadius = computeRadius(Math.abs(mToDegrees - mFromDegrees), getChildCount(),
                mChildSize, mChildPadding, MIN_RADIUS);
        final int start = mExpanded ? 0 : mRadius;
        final int radius = mExpanded ? mRadius : -mRadius;
        mRunner.start(start, 0, radius, 0, duration);
    }

    public boolean isMoving() {
        return isMoving;
    }

    @Override
    public void onMove(int lastX, int lastY, int curX, int curY) {
        layoutItem(curX);
    }

    public void onDone() {
        isMoving = false;
        if (!mExpanded) {
            FloatMenu floatMenu = (FloatMenu) getParent();
            floatMenu.remove();
        }
    }

    public boolean isExpanded() {
        return mExpanded;
    }

    /**
     * Set the arc
     */
    public void setArc(float fromDegrees, float toDegrees, int position) {
        this.position = position;
        if (mFromDegrees == fromDegrees && mToDegrees == toDegrees) {
            return;
        }
        mFromDegrees = fromDegrees;
        mToDegrees = toDegrees;
        computeCenterXY(position);
        requestLayout();
    }

    /**
     * Set the arc
     */
    public void setArc(float fromDegrees, float toDegrees) {
        setArc(fromDegrees, toDegrees, position);
    }

    /**
     * Set the submenu item size
     */
    public void setChildSize(int size) {
        mChildSize = size;
    }

    public int getChildSize() {
        return mChildSize;
    }

    public void setExpand(boolean expand) {
        mExpanded = expand;
    }
}
