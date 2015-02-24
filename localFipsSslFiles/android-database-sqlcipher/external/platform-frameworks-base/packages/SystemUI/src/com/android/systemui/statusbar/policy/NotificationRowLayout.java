/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.systemui.statusbar.policy;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.ObjectAnimator;
import android.animation.ValueAnimator;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.TypedArray;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.util.Log;
import android.util.Slog;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;

import com.android.systemui.R;
import com.android.systemui.SwipeHelper;

import java.util.HashMap;

public class NotificationRowLayout extends ViewGroup implements SwipeHelper.Callback {
    private static final String TAG = "NotificationRowLayout";
    private static final boolean DEBUG = false;
    private static final boolean SLOW_ANIMATIONS = DEBUG;

    private static final int APPEAR_ANIM_LEN = SLOW_ANIMATIONS ? 5000 : 250;
    private static final int DISAPPEAR_ANIM_LEN = APPEAR_ANIM_LEN;

    boolean mAnimateBounds = true;

    Rect mTmpRect = new Rect();
    int mNumRows = 0;
    int mRowHeight = 0;
    int mHeight = 0;

    HashMap<View, ValueAnimator> mAppearingViews = new HashMap<View, ValueAnimator>();
    HashMap<View, ValueAnimator> mDisappearingViews = new HashMap<View, ValueAnimator>();

    private SwipeHelper mSwipeHelper;

    // Flag set during notification removal animation to avoid causing too much work until
    // animation is done
    boolean mRemoveViews = true;

    public NotificationRowLayout(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public NotificationRowLayout(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.NotificationRowLayout,
                defStyle, 0);
        mRowHeight = a.getDimensionPixelSize(R.styleable.NotificationRowLayout_rowHeight, 0);
        a.recycle();

        setLayoutTransition(null);

        if (DEBUG) {
            setOnHierarchyChangeListener(new ViewGroup.OnHierarchyChangeListener() {
                @Override
                public void onChildViewAdded(View parent, View child) {
                    Slog.d(TAG, "view added: " + child + "; new count: " + getChildCount());
                }
                @Override
                public void onChildViewRemoved(View parent, View child) {
                    Slog.d(TAG, "view removed: " + child + "; new count: " + (getChildCount() - 1));
                }
            });

            setBackgroundColor(0x80FF8000);
        }

        float densityScale = getResources().getDisplayMetrics().density;
        float pagingTouchSlop = ViewConfiguration.get(mContext).getScaledPagingTouchSlop();
        mSwipeHelper = new SwipeHelper(SwipeHelper.X, this, densityScale, pagingTouchSlop);
    }

    public void setAnimateBounds(boolean anim) {
        mAnimateBounds = anim;
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        if (DEBUG) Log.v(TAG, "onInterceptTouchEvent()");
        return mSwipeHelper.onInterceptTouchEvent(ev) ||
            super.onInterceptTouchEvent(ev);
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        return mSwipeHelper.onTouchEvent(ev) ||
            super.onTouchEvent(ev);
    }

    public boolean canChildBeDismissed(View v) {
        final View veto = v.findViewById(R.id.veto);
        return (veto != null && veto.getVisibility() != View.GONE);
    }

    public void onChildDismissed(View v) {
        final View veto = v.findViewById(R.id.veto);
        if (veto != null && veto.getVisibility() != View.GONE && mRemoveViews) {
            veto.performClick();
        }
    }

    public void onBeginDrag(View v) {
        // We need to prevent the surrounding ScrollView from intercepting us now;
        // the scroll position will be locked while we swipe
        requestDisallowInterceptTouchEvent(true);
    }

    public void onDragCancelled(View v) {
    }

    public View getChildAtPosition(MotionEvent ev) {
        // find the view under the pointer, accounting for GONE views
        final int count = getChildCount();
        int y = 0;
        int touchY = (int) ev.getY();
        int childIdx = 0;
        View slidingChild;
        for (; childIdx < count; childIdx++) {
            slidingChild = getChildAt(childIdx);
            if (slidingChild.getVisibility() == GONE) {
                continue;
            }
            y += mRowHeight;
            if (touchY < y) return slidingChild;
        }
        return null;
    }

    public View getChildContentView(View v) {
        return v;
    }

    @Override
    protected void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        float densityScale = getResources().getDisplayMetrics().density;
        mSwipeHelper.setDensityScale(densityScale);
        float pagingTouchSlop = ViewConfiguration.get(mContext).getScaledPagingTouchSlop();
        mSwipeHelper.setPagingTouchSlop(pagingTouchSlop);
    }

    //**
    @Override
    public void addView(View child, int index, LayoutParams params) {
        super.addView(child, index, params);

        final View childF = child;

        if (mAnimateBounds) {
            final ObjectAnimator alphaFade = ObjectAnimator.ofFloat(child, "alpha", 0f, 1f);
            alphaFade.setDuration(APPEAR_ANIM_LEN);
            alphaFade.addListener(new AnimatorListenerAdapter() {
                @Override
                public void onAnimationEnd(Animator animation) {
                    mAppearingViews.remove(childF);
                    requestLayout(); // pick up any final changes in position
                }
            });

            alphaFade.start();

            mAppearingViews.put(child, alphaFade);

            requestLayout(); // start the container animation
        }
    }

    /**
     * Sets a flag to tell us whether to actually remove views. Removal is delayed by setting this
     * to false during some animations to smooth out performance. Callers should restore the
     * flag to true after the animation is done, and then they should make sure that the views
     * get removed properly.
     */
    public void setViewRemoval(boolean removeViews) {
        mRemoveViews = removeViews;
    }

    public void dismissRowAnimated(View child) {
        dismissRowAnimated(child, 0);
    }

    public void dismissRowAnimated(View child, int vel) {
        mSwipeHelper.dismissChild(child, vel);
    }

    @Override
    public void removeView(View child) {
        if (!mRemoveViews) {
            // This flag is cleared during an animation that removes all notifications. There
            // should be a call to remove all notifications when the animation is done, at which
            // time the view will be removed.
            return;
        }
        if (mAnimateBounds) {
            if (mAppearingViews.containsKey(child)) {
                mAppearingViews.remove(child);
            }

            // Don't fade it out if it already has a low alpha value, but run a non-visual
            // animation which is used by onLayout() to animate shrinking the gap that it left
            // in the list
            ValueAnimator anim;
            float currentAlpha = child.getAlpha();
            if (currentAlpha > .1) {
                anim = ObjectAnimator.ofFloat(child, "alpha", currentAlpha, 0);
            } else {
                if (currentAlpha > 0) {
                    // Just make it go away - no need to render it anymore
                    child.setAlpha(0);
                }
                anim = ValueAnimator.ofFloat(0, 1);
            }
            anim.setDuration(DISAPPEAR_ANIM_LEN);
            final View childF = child;
            anim.addListener(new AnimatorListenerAdapter() {
                @Override
                public void onAnimationEnd(Animator animation) {
                    if (DEBUG) Slog.d(TAG, "actually removing child: " + childF);
                    NotificationRowLayout.super.removeView(childF);
                    mDisappearingViews.remove(childF);
                    requestLayout(); // pick up any final changes in position
                }
            });

            anim.start();
            mDisappearingViews.put(child, anim);

            requestLayout(); // start the container animation
        } else {
            super.removeView(child);
        }
    }
    //**

    @Override
    public void onFinishInflate() {
        super.onFinishInflate();
        setWillNotDraw(false);
    }

    @Override
    public void onDraw(android.graphics.Canvas c) {
        super.onDraw(c);
        if (DEBUG) {
            //Slog.d(TAG, "onDraw: canvas height: " + c.getHeight() + "px; measured height: "
            //        + getMeasuredHeight() + "px");
            c.save();
            c.clipRect(6, 6, c.getWidth() - 6, getMeasuredHeight() - 6,
                    android.graphics.Region.Op.DIFFERENCE);
            c.drawColor(0xFFFF8000);
            c.restore();
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        final int count = getChildCount();

        // pass 1: count the number of non-GONE views
        int numRows = 0;
        for (int i = 0; i < count; i++) {
            final View child = getChildAt(i);
            if (child.getVisibility() == GONE) {
                continue;
            }
            if (mDisappearingViews.containsKey(child)) {
                continue;
            }
            numRows++;
        }
        if (numRows != mNumRows) {
            // uh oh, now you made us go and do work
            
            final int computedHeight = numRows * mRowHeight;
            if (DEBUG) {
                Slog.d(TAG, String.format("rows went from %d to %d, resizing to %dpx",
                            mNumRows, numRows, computedHeight));
            }

            mNumRows = numRows;

            if (mAnimateBounds && isShown()) {
                ObjectAnimator.ofInt(this, "forcedHeight", computedHeight)
                    .setDuration(APPEAR_ANIM_LEN)
                    .start();
            } else {
                setForcedHeight(computedHeight);
            }
        }

        // pass 2: you know, do the measuring
        final int childWidthMS = widthMeasureSpec;
        final int childHeightMS = MeasureSpec.makeMeasureSpec(
                mRowHeight, MeasureSpec.EXACTLY);

        for (int i = 0; i < count; i++) {
            final View child = getChildAt(i);
            if (child.getVisibility() == GONE) {
                continue;
            }

            child.measure(childWidthMS, childHeightMS);
        }

        setMeasuredDimension(
                getDefaultSize(getSuggestedMinimumWidth(), widthMeasureSpec),
                resolveSize(getForcedHeight(), heightMeasureSpec));
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        final int width = right - left;
        final int height = bottom - top;

        if (DEBUG) Slog.d(TAG, "onLayout: height=" + height);

        final int count = getChildCount();
        int y = 0;
        for (int i = 0; i < count; i++) {
            final View child = getChildAt(i);
            if (child.getVisibility() == GONE) {
                continue;
            }
            float progress = 1.0f;
            if (mDisappearingViews.containsKey(child)) {
                progress = 1.0f - mDisappearingViews.get(child).getAnimatedFraction();
            } else if (mAppearingViews.containsKey(child)) {
                progress = 1.0f - mAppearingViews.get(child).getAnimatedFraction();
            }
            if (progress > 1.0f) {
                if (DEBUG) {
                    Slog.w(TAG, "progress=" + progress + " > 1!!! " + child);
                }
                progress = 1f;
            }
            final int thisRowHeight = (int)(progress * mRowHeight);
            if (DEBUG) {
                Slog.d(TAG, String.format(
                            "laying out child #%d: (0, %d, %d, %d) h=%d",
                            i, y, width, y + thisRowHeight, thisRowHeight));
            }
            child.layout(0, y, width, y + thisRowHeight);
            y += thisRowHeight;
        }
        if (DEBUG) {
            Slog.d(TAG, "onLayout: final y=" + y);
        }
    }

    public void setForcedHeight(int h) {
        if (DEBUG) Slog.d(TAG, "forcedHeight: " + h);
        if (h != mHeight) {
            mHeight = h;
            requestLayout();
        }
    }

    public int getForcedHeight() {
        return mHeight;
    }
}
