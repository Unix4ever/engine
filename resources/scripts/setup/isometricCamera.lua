return function(self)
  local settings = self:script().state

  local movementVector = Vector3.ZERO
  local speed = settings.speed or 2
  local mouseSensivity = settings.mouseSensivity or 0.005

  local render = self:render()
  local yawNode = render.root:getSceneNode("yaw")
  local pitchNode = yawNode:getSceneNode("pitch")
  local rollNode = pitchNode:getSceneNode("roll")

  local onTime = function(delta)
    self:render().root:translate(yawNode.orientation * pitchNode.orientation * movementVector)
  end

  local handleKeyEvent = function(event)
    if event.key == Keys.KC_S then
      movementVector.z = speed
    elseif event.key == Keys.KC_W then
      movementVector.z = -speed
    end

    if event.key == Keys.KC_A then
      movementVector.x = -speed
    elseif event.key == Keys.KC_D then
      movementVector.x = speed
    end
  end

  local handleKeyUP = function(event)
    if event.key == Keys.KC_S and movementVector.z > 0 or event.key == Keys.KC_W and movementVector.z < 0 then
      movementVector.z = 0
    end
    if event.key == Keys.KC_A and movementVector.x < 0 or event.key == Keys.KC_D and movementVector.x > 0 then
      movementVector.x = 0
    end
  end

  local rotate = false

  local handleMouseEvent = function(event)
    if event.type == MouseEvent.MOUSE_DOWN and event.button == MouseEvent.Right then
      rotate = true
    elseif event.type == MouseEvent.MOUSE_UP and event.button == MouseEvent.Right then
      rotate = false
    elseif event.type == MouseEvent.MOUSE_MOVE and rotate then
      yawNode:yaw(Degree.new(-event.relX * mouseSensivity))
      pitchNode:pitch(Degree.new(-event.relY * mouseSensivity))
    end
  end


  core:script():addUpdateListener(onTime)
  event:onKey(core, KeyboardEvent.KEY_DOWN, handleKeyEvent)
  event:onKey(core, KeyboardEvent.KEY_UP, handleKeyUP)
  event:onMouse(core, MouseEvent.MOUSE_MOVE, handleMouseEvent)
  event:onMouse(core, MouseEvent.MOUSE_UP, handleMouseEvent)
  event:onMouse(core, MouseEvent.MOUSE_DOWN, handleMouseEvent)
end
