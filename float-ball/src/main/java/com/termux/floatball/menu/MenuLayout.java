package com.termux.floatball.menu;

import android.content.Context;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.ViewGroup;

import com.termux.floatball.runner.ICarrier;
import com.termux.floatball.runner.ScrollRunner;
import com.termux.floatball.utils.DensityUtil;


/**
 * 子菜单项布局
 *
 * @author 何凌波
 */
public class MenuLayout extends ViewGroup implements ICarrier {
    private int mChildSize;
    private int mChildPadding = 5;
    private float mFromDegrees;
    private float mToDegrees;
    private static int MIN_RADIUS;
    private int mRadius;// 中心菜单圆点到子菜单中心的距离
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
            case FloatMenu.LEFT_TOP://左上
                centerX = width / 2 - getRadiusAndPadding();
                centerY = height / 2 - getRadiusAndPadding();
                break;
            case FloatMenu.LEFT_CENTER://左中
                centerX = width / 2 - getRadiusAndPadding();
                centerY = height / 2;
                break;
            case FloatMenu.LEFT_BOTTOM://左下
                centerX = width / 2 - getRadiusAndPadding();
                centerY = height / 2 + getRadiusAndPadding();
                break;
            case FloatMenu.CENTER_TOP://上中
                centerX = width / 2;
                centerY = height / 2 - getRadiusAndPadding();
                break;
            case FloatMenu.CENTER_BOTTOM://下中
                centerX = width / 2;
                centerY = height / 2 + getRadiusAndPadding();
                break;
            case FloatMenu.RIGHT_TOP://右上
                centerX = width / 2 + getRadiusAndPadding();
                centerY = height / 2 - getRadiusAndPadding();
                break;
            case FloatMenu.RIGHT_CENTER://右中
                centerX = width / 2 + getRadiusAndPadding();
                centerY = height / 2;
                break;
            case FloatMenu.RIGHT_BOTTOM://右下
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
        MIN_RADIUS = DensityUtil.dip2px(context, 65);
        mRunner = new ScrollRunner(this);
        setChildrenDrawingOrderEnabled(true);
    }

    /**
     * 计算半径
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
     * 计算子菜单项的范围
     */
    private static Rect computeChildFrame(final int centerX, final int centerY, final int radius, final float degrees, final int size) {
        //子菜单项中心点
        final double childCenterX = centerX + radius * Math.cos(Math.toRadians(degrees));
        final double childCenterY = centerY + radius * Math.sin(Math.toRadians(degrees));
        //子菜单项的左上角，右上角，左下角，右下角
        return new Rect((int) (childCenterX - size / 2),
                (int) (childCenterY - size / 2), (int) (childCenterX + size / 2), (int) (childCenterY + size / 2));
    }

    /**
     * 子菜单项大小
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
     * 子菜单项位置
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
        //当悬浮球在右侧时，使其菜单从上到下的顺序和在左边时一样。
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
     * 切换中心按钮的展开缩小
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
     * 设定弧度
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
     * 设定弧度
     */
    public void setArc(float fromDegrees, float toDegrees) {
        setArc(fromDegrees, toDegrees, position);
    }

    /**
     * 设定子菜单项大小
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
