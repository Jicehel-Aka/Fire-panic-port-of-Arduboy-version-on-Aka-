idf_component_register(
    SRCS "main.cpp" "arduboy_compat/Arduboy2.cpp" "arduboy_compat/ArduboyTones.cpp" "arduboy_compat/eeprom.cpp" "game/Game.cpp" "game/entities/Ambulance.cpp" "game/entities/Angel.cpp" "game/entities/Coin.cpp" "game/entities/OtherCar.cpp" "game/entities/Player.cpp" "game/entities/Victim.cpp" "game/states/BaseState.cpp" "game/states/GameIntroState.cpp" "game/states/HighScoreState.cpp" "game/states/PlayGameState.cpp" "game/states/RaceState.cpp" "game/states/SplashScreenState.cpp" "game/states/TitleScreenState.cpp" "game/utils/Arduboy2Ext.cpp" "game/utils/EEPROM_Utils.cpp" "game/utils/GameContext.cpp"
    INCLUDE_DIRS "." "arduboy_compat"
    REQUIRES gamebuino aka_runtime
    PRIV_REQUIRES app_update)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error=reorder -Wno-error=switch -Wno-error=unused-variable
    -Wno-error=type-limits -Wno-error=narrowing -Wno-error=parentheses
    -Wno-error=maybe-uninitialized -Wno-error=unused-function -Wno-error=missing-field-initializers
    -Wno-error=unused-value -Wno-error=char-subscripts -Wno-error=unused-parameter
    -Wno-error=sign-compare)
