import com.greensock.TimelineLite;
import com.greensock.TimelineMax;
import com.greensock.easing.*;

// IDRC_TargetReticle.as, IDRC_Widgets.fla, and IDRC_Assets0.fla are based on 
// TDM_TargetLockReticle.as, TDM_Widgets.fla and TDM_Assets0.fla from 'True Directional Movement':
// https://github.com/ersh1/TrueDirectionalMovement
// All credits go to the original author Ersh!

class Widgets.IDRC_TargetReticle extends MovieClip
{
	public var ReticleOuter: MovieClip;

    private static var RETICLE_TYPE_CROSS = "cross";
    private static var RETICLE_TYPE_CIRCLE = "circle";

    var widgetReady: Boolean = false;

	var reticleStyle: Number = 0; //  0: selected actor, 1: combat target found, 2: combat target searching 
    var reticleMode: Number = 0; //  0: Hidden, 1: Cross, 2: Lock
    var circleInitZoom: Number = 1.2;

    var idleTimeline: TimelineMax;
    var pendingRemoval: Boolean = false;
    var widgetReadyToRemove: Boolean = false;

    var moveTimeline: TimelineLite;

    var crossShrinkTimeline: TimelineLite;
    var crossShrinkFromHiddenTimeline: TimelineLite;
    var crossShowTimeline: TimelineLite;
    var crossReadyToRemove: Boolean = false;
    
    var circleShowTimeline: TimelineLite;
    var circleReadyToRemove: Boolean = false;

	public function IDRC_TargetReticle() 
	{
		this.stop();

        idleTimeline = new TimelineMax({paused:true, repeat:-1, yoyo:true});
        moveTimeline = new TimelineLite({paused:true, onComplete:function() { this.reverse() }});

        crossShrinkTimeline = new TimelineLite({paused:true});
        crossShrinkFromHiddenTimeline = new TimelineLite({paused:true});
        crossShowTimeline = new TimelineLite({paused:true});
        
        circleShowTimeline = new TimelineLite({paused:true});

        pendingRemoval = false;
        widgetReadyToRemove = false;
        crossReadyToRemove = true;
        circleReadyToRemove = true;

		// delay init till everything's ready
		this.onEnterFrame = function(): Void {
            hideAll();
            
            setupIdleTimeline();
            idleTimeline.play();

            setupCrossShowTimeline();
            setupCircleShowTimeline();

            setupMoveTimeline();

            setupCrossShrinkTimeline();
            setupCrossShrinkFromHiddenTimeline();
            
            setAlpha(RETICLE_TYPE_CROSS, 0, 0);
            setAlpha(RETICLE_TYPE_CIRCLE, 0, 0);
            setVisible(RETICLE_TYPE_CROSS, 0, true);
            setVisible(RETICLE_TYPE_CIRCLE, 0, true);

            widgetReady = true;

            delete this.onEnterFrame;		
		}
	}

    public function updateReticle(a_reticleLocked: Boolean, a_TDMLocked: Boolean, a_reticleStyle)
    {
        var update: Boolean = false;

        if (!widgetReady) return;

        if (pendingRemoval) return;

        if (a_reticleStyle != reticleStyle) // reticle style has changed
        {
            updateReticleStyle(a_reticleStyle);
            update = true;
        }
        
        var newReticleMode = reticleMode;

        if (a_reticleLocked && reticleMode != 2) // not already in 'Lock' mode
        {
            newReticleMode = 2;
            update = true;
        }
        if (!a_reticleLocked)
        {
            if (a_TDMLocked && reticleMode != 0) // not already in 'Hidden' mode
            {
                newReticleMode = 0;
                update = true;
            }
            if (!a_TDMLocked && reticleMode != 1) // not already in "Cross" mode
            {
                newReticleMode = 1;
                update = true;
            }
        }

        if (update)
        {
            playTimeline(newReticleMode);
            reticleMode = newReticleMode;     
        }
    }

    private function playTimeline(a_reticleMode)
    {
        if (!widgetReady) return;

        switch(a_reticleMode) {
            case 0: // 'Hidden'
                if (reticleMode == 1) // transition from 'Cross'
                {
                    showCross(false);
                }
                if (reticleMode == 2) // transition from 'Lock'
                {
                    shrinkCrossFromHidden(false);
                    showCircle(false);
                }
                break;
            case 1: // 'Cross'
                if (reticleMode == 0) // transition from 'Hidden'
                {
                    showCross(true);
                }
                if (reticleMode == 2) // transition from 'Lock'
                {
                    shrinkCross(false);
                    showCircle(false);
                }
                break;
            case 2:  // 'Lock'
                if (reticleMode == 0) // transition from 'Hidden'
                {
                    shrinkCrossFromHidden(true);
                    showCircle(true);
                }
                if (reticleMode == 1) // transition from 'Cross'
                {
                    shrinkCross(true);
                    showCircle(true);
                }
                break;        
        }
    }

	public function changeTarget()
    {
        if (!widgetReady) return;

        // do not trigger shrink / unshrink cross in 'Lock' mode or any of the other cross timelines is ongoing
        if (reticleMode == 2 || !crossShrinkTimeline.paused() || !crossShrinkFromHiddenTimeline.paused() || !crossShowTimeline.paused()) return; 

		if (moveTimeline.progress() != 1)
		{
			moveTimeline.play();
		}
		else
		{
			moveTimeline.reverse();
		}
	}

	public function updateWidgetState(a_bPendingRemoval: Boolean)
    {
        if (!widgetReady) return;

		if (a_bPendingRemoval != pendingRemoval)
		{
            pendingRemoval = a_bPendingRemoval;

			if (a_bPendingRemoval) {
                playTimeline(0);
                reticleMode = 0;
			}
		}
	}

    private function shrinkCross(a_shrink: Boolean)
    {
        if (!widgetReady) return;
        
        // pause all other cross timelines
        crossShrinkFromHiddenTimeline.pause();
        crossShowTimeline.pause();
        moveTimeline.pause();

        if (a_shrink)
        {
            crossShrinkTimeline.progress(0);
            crossShrinkTimeline.play();
        } else
        {
            crossShrinkTimeline.progress(1);
            crossShrinkTimeline.reverse();
        }
    }

    private function shrinkCrossFromHidden(a_shrink: Boolean)
    {
        if (!widgetReady) return;

        // pause all other cross timelines
        crossShrinkTimeline.pause();
        crossShowTimeline.pause();
        moveTimeline.pause();

        if (a_shrink)
        {
            crossShrinkFromHiddenTimeline.progress(0);
            crossShrinkFromHiddenTimeline.play();
        } else
        {
            crossShrinkFromHiddenTimeline.progress(1);
            crossShrinkFromHiddenTimeline.reverse();
        }
    }

    private function showCross(a_show: Boolean) 
    {
        if (!widgetReady) return;

        // pause all other cross timelines
        crossShrinkTimeline.pause();
        crossShrinkFromHiddenTimeline.pause();
        moveTimeline.pause();

        if (a_show)
        {
            crossShowTimeline.progress(0);
            crossShowTimeline.play();
        } else
        {
            crossShowTimeline.progress(1);
            crossShowTimeline.reverse();
        }
    }

    private function showCircle(a_show: Boolean) 
    {
        if (!widgetReady) return;

        if (a_show)
        {
            circleShowTimeline.progress(0);
            circleShowTimeline.play();
        } else
        {
            circleShowTimeline.progress(1);
            circleShowTimeline.reverse();
        }       
    }  

	private function updateReticleStyle(a_reticleStyle: Number)
	{
        if (!widgetReady)
        {
            return;
        }

        if (reticleStyle == a_reticleStyle)
        {
            return;
        }
		reticleStyle = a_reticleStyle;

        hideAll();
        setVisible(RETICLE_TYPE_CROSS, reticleStyle, true);
        setVisible(RETICLE_TYPE_CIRCLE, reticleStyle, true);
    }

    public function SetCircleInitZoom(a_zoom: Number)
    {
        if (circleInitZoom == a_zoom) return;

        circleInitZoom = a_zoom;
        if (circleInitZoom < 0.1)
        {
            circleInitZoom = 0.1;
        }
        if (circleInitZoom > 5.0)
        {
            circleInitZoom = 5.0;
        }

        setupCircleShowTimeline();
    }

    private function setupIdleTimeline()
    {
        idleTimeline.clear();
        idleTimeline.fromTo(ReticleOuter, 0.5, {_xscale:100, _yscale:100}, {_xscale:110, _yscale:110}, 0);
    }

    private function setupCrossShowTimeline()
    {
        crossShowTimeline.clear();
        crossShowTimeline.eventCallback("onReverseComplete", setCrossReadyToRemove, [true], this);
        crossShowTimeline.eventCallback("onComplete", setCrossReadyToRemove, [false], this);

        setupCrossShowTimelineForStyle(0); // Selected actor (from screen center)
        setupCrossShowTimelineForStyle(1); // combat target - found
        setupCrossShowTimelineForStyle(2); // combat target - searching
    }

    private function setupCircleShowTimeline()
    {
        circleShowTimeline.clear();
        circleShowTimeline.eventCallback("onReverseComplete", setCircleReadyToRemove, [true], this);
        circleShowTimeline.eventCallback("onComplete", setCircleReadyToRemove, [false], this);

        setupCircleShowTimelineForStyle(0); // Selected actor (from screen center)
        setupCircleShowTimelineForStyle(1); // combat target - found
        setupCircleShowTimelineForStyle(2); // combat target - searching
    }

    private function setupMoveTimeline()
    {
        moveTimeline.clear();

        setupMoveTimelineForStyle(0); // Selected actor (from screen center)
        setupMoveTimelineForStyle(1); // combat target - found
        setupMoveTimelineForStyle(2); // combat target - searching
    }

    private function setupCrossShrinkTimeline()
    {
        crossShrinkTimeline.clear();
        crossShrinkTimeline.eventCallback("onComplete", setCrossReadyToRemove, [false], this);
       
        setupCrossShrinkTimelineForStyle(0);
        setupCrossShrinkTimelineForStyle(1);
        setupCrossShrinkTimelineForStyle(2);
    }

    private function setupCrossShrinkFromHiddenTimeline()
    {
        crossShrinkFromHiddenTimeline.clear();
        crossShrinkFromHiddenTimeline.eventCallback("onReverseComplete", setCrossReadyToRemove, [true], this);
        crossShrinkFromHiddenTimeline.eventCallback("onComplete", setCrossReadyToRemove, [false], this);
       
        setupCrossShrinkFromHiddenTimelineForStyle(0);
        setupCrossShrinkFromHiddenTimelineForStyle(1);
        setupCrossShrinkFromHiddenTimelineForStyle(2);
    }

    private function setupCrossShowTimelineForStyle (a_reticleStyle)
    {
        var parts = getReticleParts(RETICLE_TYPE_CROSS, a_reticleStyle);
        if (!parts || !parts.top || !parts.bottom || !parts.left || !parts.right) return;  
        
        crossShowTimeline.set(parts.top, {_rotation:-180}, 0);
        crossShowTimeline.set(parts.bottom, {_rotation:0}, 0);
        crossShowTimeline.set(parts.left, {_rotation:-270}, 0);
        crossShowTimeline.set(parts.right, {_rotation:-90}, 0);

        crossShowTimeline.fromTo(parts.top, 0.5, {_alpha:0, _y:-7}, {_alpha:100, _y:-15, ease:Sine.easeInOut}, 0);
        crossShowTimeline.fromTo(parts.bottom, 0.5, {_alpha:0, _y:7}, {_alpha:100, _y:15, ease:Sine.easeInOut}, 0);
        crossShowTimeline.fromTo(parts.left, 0.5, {_alpha:0, _x:-7}, {_alpha:100, _x:-15, ease:Sine.easeInOut}, 0);
        crossShowTimeline.fromTo(parts.right, 0.5, {_alpha:0, _x:7}, {_alpha:100, _x:15, ease:Sine.easeInOut}, 0);
    }

    private function setupCircleShowTimelineForStyle(a_reticleStyle)
    {
        var parts = getReticleParts(RETICLE_TYPE_CIRCLE, a_reticleStyle);
        if (!parts || !parts.top || !parts.bottom || !parts.left || !parts.right) return;  

        circleShowTimeline.set(parts.top, {_rotation:180}, 0);     // Upper left
        circleShowTimeline.set(parts.bottom, {_rotation:0}, 0);    // Lower right
        circleShowTimeline.set(parts.left, {_rotation:90}, 0);     // Lower left
        circleShowTimeline.set(parts.right, {_rotation:-90}, 0);   // Upper right

        var zoomCoord: Number = circleInitZoom * 15;
        var zoomScale: Number = circleInitZoom * 150;

        // Rotate 90 clockwise, zoom, and fade in
        circleShowTimeline.fromTo(parts.top, 0.5, 
            {_alpha:0, _x:-zoomCoord, _y:-0.5*zoomCoord, _rotation:90, _xscale:zoomScale, _yscale:zoomScale}, 
            {_alpha:100, _x:0, _y:-15, _rotation:180, _xscale:100, _yscale:100, ease:Sine.easeInOut}, 0);
        circleShowTimeline.fromTo(parts.bottom, 0.5, 
            {_alpha:0, _x:zoomCoord, _y:0.5*zoomCoord, _rotation:-90, _xscale:zoomScale, _yscale:zoomScale}, 
            {_alpha:100, _x:0, _y:15, _rotation:0, _xscale:100, _yscale:100, ease:Sine.easeInOut}, 0);
        circleShowTimeline.fromTo(parts.left, 0.5, 
            {_alpha:0, _x:-0.5*zoomCoord, _y:zoomCoord, _rotation:0, _xscale:zoomScale, _yscale:zoomScale}, 
            {_alpha:100, _x:-15, _y:0, _rotation:90, _xscale:100, _yscale:100, ease:Sine.easeInOut}, 0);
        circleShowTimeline.fromTo(parts.right, 0.5, 
            {_alpha:0, _x:0.5*zoomCoord, _y:-zoomCoord, _rotation:180, _xscale:zoomScale, _yscale:zoomScale}, 
            {_alpha:100, _x:15, _y:0, _rotation:270, _xscale:100, _yscale:100, ease:Sine.easeInOut}, 0);        
    }

    private function setupMoveTimelineForStyle(a_reticleStyle)
    {
        var parts = getReticleParts(RETICLE_TYPE_CROSS, a_reticleStyle);
        if (!parts || !parts.top || !parts.bottom || !parts.left || !parts.right) return;  
       
        moveTimeline.fromTo(parts.top, 0.25, {_y:-15}, {_y:-7}, 0);
        moveTimeline.fromTo(parts.bottom, 0.25, {_y:15}, {_y:7}, 0);
        moveTimeline.fromTo(parts.left, 0.25, {_x:-15}, {_x:-7}, 0);
        moveTimeline.fromTo(parts.right, 0.25, {_x:15}, {_x:7}, 0);        
    }

    private function setupCrossShrinkTimelineForStyle(a_reticleStyle)
    {        
        var parts = getReticleParts(RETICLE_TYPE_CROSS, a_reticleStyle);
        if (!parts || !parts.top || !parts.bottom || !parts.left || !parts.right) return;        

        crossShrinkTimeline.fromTo(parts.top, 1.0, {_alpha:100, _y:-15}, {_alpha:100, _y:-7, ease:Sine.easeInOut}, 0);
        crossShrinkTimeline.fromTo(parts.bottom, 1.0, {_alpha:100, _y:15}, {_alpha:100, _y:7, ease:Sine.easeInOut}, 0);
        crossShrinkTimeline.fromTo(parts.left, 1.0, {_alpha:100, _x:-15}, {_alpha:100, _x:-7, ease:Sine.easeInOut}, 0);
        crossShrinkTimeline.fromTo(parts.right, 1.0, {_alpha:100, _x:15}, {_alpha:100, _x:7, ease:Sine.easeInOut}, 0);

/* Alternative shrinkTimeline with cross rotating while shrinking
        crossShrinkTimeline.fromTo(parts.top, 1.0, {_alpha:100, _x:0, _y:-15, _rotation:180}, 
                        {_alpha:100, _x:-7, _y:0,  _rotation:90, ease:Sine.easeInOut}, 0);
        crossShrinkTimeline.fromTo(parts.bottom, 1.0, {_alpha:100, _x:0, _y:15, _rotation:0}, 
                        {_alpha:100, _x:7, _y:0, _rotation:-90, ease:Sine.easeInOut}, 0);
        crossShrinkTimeline.fromTo(parts.left, 1.0, {_alpha:100, _x:-15, _y:0, _rotation:90}, 
                        {_alpha:100, _x:0, _y:7, _rotation:0}, ease:Sine.easeInOut, 0);
        crossShrinkTimeline.fromTo(parts.right, 1.0, {_alpha:100, _x:15, _y:0 , _rotation:-90}, 
                        {_alpha:100, _x:0, _y:-7, _rotation:-180, ease:Sine.easeInOut}, 0);
*/       
    }

    private function setupCrossShrinkFromHiddenTimelineForStyle(a_reticleStyle)
    {        
        var parts = getReticleParts(RETICLE_TYPE_CROSS, a_reticleStyle);
        if (!parts || !parts.top || !parts.bottom || !parts.left || !parts.right) return;        

        crossShrinkFromHiddenTimeline.fromTo(parts.top, 1.0, {_alpha:0, _y:-15}, {_alpha:100, _y:-7, ease:Sine.easeInOut}, 0);
        crossShrinkFromHiddenTimeline.fromTo(parts.bottom, 1.0, {_alpha:0, _y:15}, {_alpha:100, _y:7, ease:Sine.easeInOut}, 0);
        crossShrinkFromHiddenTimeline.fromTo(parts.left, 1.0, {_alpha:0, _x:-15}, {_alpha:100, _x:-7, ease:Sine.easeInOut}, 0);
        crossShrinkFromHiddenTimeline.fromTo(parts.right, 1.0, {_alpha:0, _x:15}, {_alpha:100, _x:7, ease:Sine.easeInOut}, 0);

/* Alternative shrinkTimeline with cross rotating while shrinking
        crossShrinkFromHiddenTimeline.fromTo(parts.top, 1.0, {_alpha:0, _x:0, _y:-15, _rotation:180}, 
                        {_alpha:100, _x:-7, _y:0,  _rotation:90, ease:Sine.easeInOut}, 0);
        crossShrinkFromHiddenTimeline.fromTo(parts.bottom, 1.0, {_alpha:0, _x:0, _y:15, _rotation:0}, 
                        {_alpha:100, _x:7, _y:0, _rotation:-90, ease:Sine.easeInOut}, 0);
        crossShrinkFromHiddenTimeline.fromTo(parts.left, 1.0, {_alpha:0, _x:-15, _y:0, _rotation:90}, 
                        {_alpha:100, _x:0, _y:7, _rotation:0, ease:Sine.easeInOut}, 0);
        crossShrinkFromHiddenTimeline.fromTo(parts.right, 1.0, {_alpha:0, _x:15, _y:0 , _rotation:-90}, 
                        {_alpha:100, _x:0, _y:-7, _rotation:-180, ease:Sine.easeInOut}, 0);
*/       
    }    

    private function hideAll()
    {
        setVisible(RETICLE_TYPE_CROSS, 0, false); // Selected actor (from screen center)
        setVisible(RETICLE_TYPE_CROSS, 1, false); // combat target - found
        setVisible(RETICLE_TYPE_CROSS, 2, false); // combat target - searching
        setVisible(RETICLE_TYPE_CIRCLE, 0, false); // Selected actor (from screen center)
        setVisible(RETICLE_TYPE_CIRCLE, 1, false); // combat target - found
        setVisible(RETICLE_TYPE_CIRCLE, 2, false); // combat target - searching
    }

    private function setVisible(a_reticleType, a_reticleStyle, a_visible: Boolean) {

        var parts = getReticleParts(a_reticleType, a_reticleStyle);
        if (!parts || !parts.top || !parts.bottom || !parts.left || !parts.right) return;  

        parts.top._visible = a_visible;
        parts.bottom._visible = a_visible;
        parts.left._visible = a_visible;
        parts.right._visible = a_visible;    
    }

     private function setAlpha(a_reticleType, a_reticleStyle, a_alpha) {

        var parts = getReticleParts(a_reticleType, a_reticleStyle);
        if (!parts || !parts.top || !parts.bottom || !parts.left || !parts.right) return;  

        parts.top._alpha = a_alpha;
        parts.bottom._alpha = a_alpha;
        parts.left._alpha = a_alpha;
        parts.right._alpha = a_alpha;    
    }   

    private function getReticleParts(a_reticleType, a_reticleStyle) {
        var parts = {};
        if (a_reticleType == RETICLE_TYPE_CROSS) {
            switch(a_reticleStyle) {
                case 0: // Selected actor (from screen center)
                    parts.top = ReticleOuter.Reticle.ReticleCrossTopBlack;
                    parts.bottom = ReticleOuter.Reticle.ReticleCrossBottomBlack;
                    parts.left = ReticleOuter.Reticle.ReticleCrossLeftBlack;
                    parts.right = ReticleOuter.Reticle.ReticleCrossRightBlack;
                    break;
                    
                case 1: // combat target - found
                    parts.top = ReticleOuter.Reticle.ReticleCrossTopRed;
                    parts.bottom = ReticleOuter.Reticle.ReticleCrossBottomRed;
                    parts.left = ReticleOuter.Reticle.ReticleCrossLeftRed;
                    parts.right = ReticleOuter.Reticle.ReticleCrossRightRed;
                    break;
                    
                case 2: // combat target - searching
                    parts.top = ReticleOuter.Reticle.ReticleCrossTopOrange;
                    parts.bottom = ReticleOuter.Reticle.ReticleCrossBottomOrange;
                    parts.left = ReticleOuter.Reticle.ReticleCrossLeftOrange;
                    parts.right = ReticleOuter.Reticle.ReticleCrossRightOrange;
                    break;
            }
        } else {
            switch(a_reticleStyle) {
                case 0: // Selected actor (from screen center)
                    parts.top = ReticleOuter.Reticle.ReticleCircleTopBlack;
                    parts.bottom = ReticleOuter.Reticle.ReticleCircleBottomBlack;
                    parts.left = ReticleOuter.Reticle.ReticleCircleLeftBlack;
                    parts.right = ReticleOuter.Reticle.ReticleCircleRightBlack;
                    break;
                    
                case 1: // combat target - found
                    parts.top = ReticleOuter.Reticle.ReticleCircleTopRed;
                    parts.bottom = ReticleOuter.Reticle.ReticleCircleBottomRed;
                    parts.left = ReticleOuter.Reticle.ReticleCircleLeftRed;
                    parts.right = ReticleOuter.Reticle.ReticleCircleRightRed;
                    break;
                    
                case 2: // combat target - searching
                    parts.top = ReticleOuter.Reticle.ReticleCircleTopOrange;
                    parts.bottom = ReticleOuter.Reticle.ReticleCircleBottomOrange;
                    parts.left = ReticleOuter.Reticle.ReticleCircleLeftOrange;
                    parts.right = ReticleOuter.Reticle.ReticleCircleRightOrange;
                    break;
            }
        }
        return parts;
    }

    public function cleanUp() {
		idleTimeline.clear();
		idleTimeline.kill();
        idleTimeline = null;

		crossShowTimeline.eventCallback("onReverseComplete", null);
		crossShowTimeline.eventCallback("onComplete", null);
		crossShowTimeline.clear();
		crossShowTimeline.kill();
        crossShowTimeline = null;

		circleShowTimeline.eventCallback("onReverseComplete", null);
		circleShowTimeline.eventCallback("onComplete", null);
		circleShowTimeline.clear();
		circleShowTimeline.kill();
        circleShowTimeline = null;

		moveTimeline.eventCallback("onComplete", null);
		moveTimeline.clear();
		moveTimeline.kill();
        moveTimeline = null;

 		crossShrinkTimeline.eventCallback("onReverseComplete", null);
		crossShrinkTimeline.eventCallback("onComplete", null);
        crossShrinkTimeline.clear();
        crossShrinkTimeline.kill();
        crossShrinkTimeline = null;

		crossShrinkFromHiddenTimeline.eventCallback("onReverseComplete", null);
		crossShrinkFromHiddenTimeline.eventCallback("onComplete", null);
        crossShrinkFromHiddenTimeline.clear();
        crossShrinkFromHiddenTimeline.kill();
        crossShrinkFromHiddenTimeline = null;

        hideAll();
        pendingRemoval = false;
        widgetReadyToRemove = false;
        crossReadyToRemove = true;
        circleReadyToRemove = true;
        widgetReady = false;
	}

    public function setWidgetReadyToRemove(a_readyToRemove: Boolean)
    {
        widgetReadyToRemove = a_readyToRemove;

        if (a_readyToRemove == false)
        {
            crossReadyToRemove = false;
            circleReadyToRemove = false;
        }
    }

    public function isReadyToRemove() : Boolean
    {
        return crossReadyToRemove && circleReadyToRemove && widgetReadyToRemove;
    }

    private function setCrossReadyToRemove(a_readyToRemove: Boolean)
    {
        crossReadyToRemove = a_readyToRemove;
    }

    private function setCircleReadyToRemove(a_readyToRemove: Boolean)
    {
        circleReadyToRemove = a_readyToRemove;
    }
}
