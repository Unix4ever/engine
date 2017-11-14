require 'lib.class'


-- base class for all windowed views
ImguiWindow = class(function(self, title, docked, open)
  self.docked = docked
  self.open = open
  self.title = title
  if self.docked then
    self.imguiBegin = function(self)
      if self.open ~= nil then
        self.open = imgui.BeginDock(self.title, self.open)
      else
        self.open = imgui.BeginDock(self.title)
      end
      if not self.open then
        self:imguiEnd()
      end
      return self.open
    end
    self.imguiEnd = function(self)  imgui.EndDock() end
  else
    self.imguiBegin = function(self)
      local open
      if self.open ~= nil then
        _, open = imgui.Begin(self.title, self.open)
      else
        _, open = imgui.Begin(self.title)
      end
      self.open = open
      return self.open
    end
    self.imguiEnd = function(self) imgui.End() end
  end
end)

-- wrap function into ImguiWindow
local ImguiWindowRender = class(ImguiWindow, function(self, callback, title, docked, open)
  ImguiWindow.init(self, title, docked, open)
  self.callback = callback
end)

-- render view
function ImguiWindowRender:__call()
  if self:imguiBegin() then
    self:callback()
    self:imguiEnd()
  end
end

-- imgui interface
local ImguiInterface = class(function(self)
  self.views = {}
end)

-- is imgui available for usage
function ImguiInterface:available()
  return imgui ~= nil and imgui.render ~= nil
end

-- add view to imgui render list
function ImguiInterface:addView(name, view, docked, open, title)
  if not self:available() then
    return false
  end

  if type(view) == "function" then
    view = ImguiWindowRender(view, title or name, docked, open)
  end

  local added = imgui.render:addView(name, view)
  if not added then
    return false
  end
  self.views[name] = view
  return true
end

-- remove view from imgui render list
function ImguiInterface:removeView(name, view)
  if not self:available() then
    return false
  end

  local removed = imgui.render:removeView(name, view)
  self.views[name] = nil
  return removed
end

-- open/close view
function ImguiInterface:setViewOpen(name, value)
  if self.views[name] then
    self.views[name].open = value
    return true
  end
  return false
end

local imguiInterface = imguiInterface or ImguiInterface()
return imguiInterface
