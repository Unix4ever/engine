package.path = package.path .. ';' .. getResourcePath('scripts/?.lua') ..
                               ';' .. getResourcePath('behaviors/trees/?.lua') ..
                               ";" .. getResourcePath('behaviors/?.lua') .. ";"

require 'math'
require 'helpers.base'
require 'lib.behaviors'
require 'actions'
require 'factories.camera'
require 'factories.emitters'
local eal = require 'lib.eal.manager'
local event = require 'lib.event'

local imguiInterface = require 'imgui.base'

if imguiInterface:available() then
  require 'imgui.menu'
  require 'imgui.console'
  require 'imgui.ogreView'
  require 'imgui.stats'
end

local rocketInitialized = false

function initLibrocket(event)
  local ctx = rocket.contexts[event.name]
  if not rocketInitialized then
    local fonts =
    {
      "Delicious-Roman.otf",
      "Delicious-BoldItalic.otf",
      "Delicious-Bold.otf",
      "Delicious-Italic.otf",
      "lucida.ttf"
    }
    for _, font in pairs(fonts) do
      resource.loadFont(font)
    end
  end

  main = resource.loadDocument(ctx, "minimal.rml")
  cursor = resource.loadCursor(ctx, "cursor.rml")

  main:Show()
end

-- librocket initialization
if event.onRocketContext ~= nil then
  event:onRocketContext(core, RocketContextEvent.CREATE, initLibrocket)
end

local selectTransform = false

function handleKeyEvent(e)
  if e.type == KeyboardEvent.KEY_UP then
    selectTransform = false
    return
  end

  if e.key == Keys.KC_T and e.type == KeyboardEvent.KEY_DOWN then
    selectTransform = true
  end
end

event:onKeyboard(core, KeyboardEvent.KEY_DOWN, handleKeyEvent)
event:onKeyboard(core, KeyboardEvent.KEY_UP, handleKeyEvent)

local initialized = false

event:bind(core, Facade.LOAD, onReady)

local function saveDockState()
  local state = imgui.GetDockState()
  editor:putToGlobalState("dockState", state)
  log.info("Saving imgui dock state")
  if not editor:saveGlobalState() then
    log.error("Failed to save global state")
  end
end

if imguiInterface:available() then
  imguiConsole = Console(256, true)
  imguiInterface:addView("console", imguiConsole)

  ogreView = OgreView("mainOgreView", "viewport", true)
  imguiInterface:addView("ogreView", ogreView)

  local function onAreaLoad(event)
    ogreView:createCamera("free")
  end

  event:bind(core, Facade.LOAD, onAreaLoad)

  imguiInterface:addView("transform", function()
    imgui.TextWrapped("coming soon")
  end, true)

  imguiInterface:addView("profiler", function()
    imgui.TextWrapped("coming soon")
  end, true)

  imguiInterface:addView("assets", function()
    imgui.TextWrapped("coming soon")
  end, true)

  local views = {}
  for name, view in pairs(imguiInterface.views) do
    views[#views+1] = MenuItem(view.title,
      function() view.open = not view.open end,
      function() return view.open end
    )
  end

  local menus = {
    List("Views", views)
  }
  imguiInterface:addView("mainMenu", Menu(menus))

  -- read saved editor settings
  local globalEditorState = editor:getGlobalState()
  if globalEditorState and globalEditorState.dockState then
    log.info("Restoring imgui dock state")
    imgui.SetDockState(globalEditorState.dockState)
  end
  event:bind(core, EngineEvent.STOPPING, saveDockState)
end

function reload()
  game:loadArea("exampleLevel")
  game:reset()
  game:loadArea("exampleLevel")
end
