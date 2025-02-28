from PySide6 import QtWidgets, QtGui, QtCore
from PySide6.QtCore import QFileSystemWatcher, QCoreApplication
from PySide6.QtWidgets import QGraphicsView, QGraphicsScene
from qt_material import apply_stylesheet
import multiprocessing
import threading
import requests
import pymem
import pymem.process
import win32api
import win32con
import win32gui
from pynput.mouse import Controller, Button
import json
import os
import shutil
import sys
import time
 
 
CONFIG_DIR = os.path.join(os.environ["LOCALAPPDATA"], "temp", "luxecs2")
CONFIG_FILE = os.path.join(CONFIG_DIR, "config.json")
DEFAULT_SETTINGS = {
    "esp_rendering": 1,
    "esp_mode": 0,
    "line_rendering": 1,
    "hp_bar_rendering": 1,
    "head_hitbox_rendering": 1,
    "bons": 1,
    "nickname": 1,
    "radius": 3,  # tamanho fov
    "keyboard": "Z", # tecla fov
    "aim_active": 1,
    "aim_mode": 1,
    "aim_mode_distance": 1,
    "keyboards": "Z",
    "weapon": 1,
    "bomb_esp": 1,
}


if os.path.exists(CONFIG_FILE):
    os.remove(CONFIG_FILE)

os.makedirs(CONFIG_DIR, exist_ok=True)
 
BombPlantedTime = 0
BombDefusedTime = 0
 
 
def load_settings():
    if not os.path.exists(CONFIG_DIR):
        os.makedirs(CONFIG_DIR)
    if not os.path.exists(CONFIG_FILE):
        with open(CONFIG_FILE, "w") as f:
            json.dump(DEFAULT_SETTINGS, f, indent=4)
    with open(CONFIG_FILE, "r") as f:
        return json.load(f)
 
 
def save_settings(settings):
    with open(CONFIG_FILE, "w") as f:
        json.dump(settings, f, indent=4)
 
 
def get_offsets_and_client_dll():
    offsets = requests.get(
        "https://luxecheatsoffsets.netlify.app/luxecheatsoffsets.json"
    ).json()
    client_dll = requests.get(
        "https://luxecheatsoffsets.netlify.app/dados.json"
    ).json()
    return offsets, client_dll
 
 
def get_window_size(window_title):
    hwnd = win32gui.FindWindow(None, window_title)
    if hwnd:
        rect = win32gui.GetClientRect(hwnd)
        return rect[2], rect[3]
    return None, None
 
 
def w2s(mtx, posx, posy, posz, width, height):
    screenW = (mtx[12] * posx) + (mtx[13] * posy) + (mtx[14] * posz) + mtx[15]
    if screenW > 0.001:
        screenX = (mtx[0] * posx) + (mtx[1] * posy) + (mtx[2] * posz) + mtx[3]
        screenY = (mtx[4] * posx) + (mtx[5] * posy) + (mtx[6] * posz) + mtx[7]
        camX = width / 2
        camY = height / 2
        x = camX + (camX * screenX / screenW)
        y = camY - (camY * screenY / screenW)
        return [int(x), int(y)]
    return [-999, -999]
 
 
class ConfigWindow(QtWidgets.QWidget):
    def __init__(self):
        super().__init__()
        self.settings = load_settings()
        self.initUI()
        self.is_dragging = False
        self.drag_start_position = None
 
    def initUI(self):
        self.setWindowFlags(QtCore.Qt.FramelessWindowHint)
        self.setFixedSize(520, 720)
 
        aim_container = self.create_aim_container()
        esp_container = self.create_esp_container()

    def create_aim_container(self):
        aim_container = QtWidgets.QWidget()
        aim_layout = QtWidgets.QVBoxLayout()
 
        aim_label = QtWidgets.QLabel("Aimbot")
        aim_label.setAlignment(QtCore.Qt.AlignCenter)
        aim_layout.addWidget(aim_label)
 
        self.aim_active_cb = QtWidgets.QCheckBox("Enable")
        self.aim_active_cb.setChecked(self.settings["aim_active"] == 1)
        self.aim_active_cb.stateChanged.connect(self.save_settings)
        aim_layout.addWidget(self.aim_active_cb)
 
        self.radius_slider = QtWidgets.QSlider(QtCore.Qt.Horizontal)
        self.radius_slider.setMinimum(0)
        self.radius_slider.setMaximum(100)
        self.radius_slider.setValue(self.settings["radius"])
        self.radius_slider.valueChanged.connect(self.save_settings)
        aim_layout.addWidget(QtWidgets.QLabel("FOV:"))
        aim_layout.addWidget(self.radius_slider)
 
        self.keyboard_input = QtWidgets.QLineEdit()
        self.keyboard_input.setText(self.settings["keyboard"])
        self.keyboard_input.textChanged.connect(self.save_settings)
        aim_layout.addWidget(QtWidgets.QLabel("Key:"))
        aim_layout.addWidget(self.keyboard_input)
        self.keyboard_input.setStyleSheet(
            "background-color: #3B4252; border-radius: 5px; color: #ECEFF4;"
        )
 
        self.aim_mode_cb = QtWidgets.QComboBox()
        self.aim_mode_cb.addItems(["Body", "Head"])
        self.aim_mode_cb.setCurrentIndex(self.settings["aim_mode"])
        self.aim_mode_cb.setStyleSheet(
            "background-color: #3B4252; border-radius: 5px; color: #ECEFF4;"
        )
        self.aim_mode_cb.currentIndexChanged.connect(self.save_settings)
        aim_layout.addWidget(QtWidgets.QLabel("Hitbox:"))
        aim_layout.addWidget(self.aim_mode_cb)
 
        self.aim_mode_distance_cb = QtWidgets.QComboBox()
        self.aim_mode_distance_cb.addItems(
            ["Closest to Crosshair", "Closest in 3D"]
        )
        self.aim_mode_distance_cb.setCurrentIndex(
            self.settings["aim_mode_distance"]
        )
        self.aim_mode_distance_cb.setStyleSheet(
            "background-color: #3B4252; border-radius: 5px; color: #ECEFF4;"
        )
        self.aim_mode_distance_cb.currentIndexChanged.connect(
            self.save_settings
        )
        aim_layout.addWidget(QtWidgets.QLabel("Aim Distance Mode:"))
        aim_layout.addWidget(self.aim_mode_distance_cb)
 
        aim_container.setLayout(aim_layout)
        aim_container.setStyleSheet(
            "background-color: #3B4252; border-radius: 10px;"
        )
        return aim_container
 
    def create_esp_container(self):
        esp_container = QtWidgets.QWidget()
        esp_layout = QtWidgets.QVBoxLayout()

        esp_label = QtWidgets.QLabel("Esp")
        esp_label.setAlignment(QtCore.Qt.AlignCenter)
        esp_layout.addWidget(esp_label)

        self.esp_rendering_cb = QtWidgets.QCheckBox("Enable")
        self.esp_rendering_cb.setChecked(self.settings["esp_rendering"] == 1)
        self.esp_rendering_cb.stateChanged.connect(self.save_settings)
        esp_layout.addWidget(self.esp_rendering_cb)

        self.esp_mode_cb = QtWidgets.QComboBox()
        self.esp_mode_cb.addItems(["Enemies Only", "All Players"])
        self.esp_mode_cb.setCurrentIndex(self.settings["esp_mode"])
        self.esp_mode_cb.setStyleSheet(
            "background-color: #3B4252; border-radius: 5px; color: #ECEFF4;"
        )
        self.esp_mode_cb.currentIndexChanged.connect(self.save_settings)
        esp_layout.addWidget(self.esp_mode_cb)

        self.line_rendering_cb = QtWidgets.QCheckBox("Lines")
        self.line_rendering_cb.setChecked(self.settings["line_rendering"] == 1)
        self.line_rendering_cb.stateChanged.connect(self.save_settings)
        esp_layout.addWidget(self.line_rendering_cb)

        self.hp_bar_rendering_cb = QtWidgets.QCheckBox("HP")
        self.hp_bar_rendering_cb.setChecked(self.settings["hp_bar_rendering"] == 1)
        self.hp_bar_rendering_cb.stateChanged.connect(self.save_settings)
        esp_layout.addWidget(self.hp_bar_rendering_cb)

        self.bons_cb = QtWidgets.QCheckBox("Skeleton")
        self.bons_cb.setChecked(self.settings["bons"] == 1)
        self.bons_cb.stateChanged.connect(self.save_settings)
        esp_layout.addWidget(self.bons_cb)

        self.nickname_cb = QtWidgets.QCheckBox("Name")
        self.nickname_cb.setChecked(self.settings["nickname"] == 1)
        self.nickname_cb.stateChanged.connect(self.save_settings)
        esp_layout.addWidget(self.nickname_cb)

        self.weapon_cb = QtWidgets.QCheckBox("Weapon")
        self.weapon_cb.setChecked(self.settings["weapon"] == 1)
        self.weapon_cb.stateChanged.connect(self.save_settings)
        esp_layout.addWidget(self.weapon_cb)

        self.bomb_esp_cb = QtWidgets.QCheckBox("Bomb")
        self.bomb_esp_cb.setChecked(self.settings["bomb_esp"] == 1)
        self.bomb_esp_cb.stateChanged.connect(self.save_settings)
        esp_layout.addWidget(self.bomb_esp_cb)

        esp_container.setLayout(esp_layout)
        esp_container.setStyleSheet(
            "background-color: #3B4252; border-radius: 10px;"
        )

        return esp_container
 
    def save_settings(self):
        self.settings["esp_rendering"] = (
            1 if self.esp_rendering_cb.isChecked() else 0
        )
        self.settings["esp_mode"] = self.esp_mode_cb.currentIndex()
        self.settings["line_rendering"] = (
            1 if self.line_rendering_cb.isChecked() else 0
        )
        self.settings["hp_bar_rendering"] = (
            1 if self.hp_bar_rendering_cb.isChecked() else 0
        )
        self.settings["head_hitbox_rendering"] = (
            1 if self.head_hitbox_rendering_cb.isChecked() else 0
        )
        self.settings["bons"] = 1 if self.bons_cb.isChecked() else 0
        self.settings["nickname"] = 1 if self.nickname_cb.isChecked() else 0
        self.settings["weapon"] = 1 if self.weapon_cb.isChecked() else 0
        self.settings["bomb_esp"] = 1 if self.bomb_esp_cb.isChecked() else 0
        self.settings["trigger_bot_active"] = (
            1 if self.trigger_bot_active_cb.isChecked() else 0
        )
        self.settings["keyboards"] = self.trigger_key_input.text()
        self.settings["aim_active"] = 1 if self.aim_active_cb.isChecked() else 0
        self.settings["radius"] = self.radius_slider.value()
        self.settings["keyboard"] = self.keyboard_input.text()
        self.settings["aim_mode"] = self.aim_mode_cb.currentIndex()
        self.settings["aim_mode_distance"] = (
            self.aim_mode_distance_cb.currentIndex()
        )
        save_settings(self.settings)
 
    def mousePressEvent(self, event: QtGui.QMouseEvent):
        if event.button() == QtCore.Qt.LeftButton:
            self.is_dragging = True
            self.drag_start_position = event.globalPosition().toPoint()
 
    def mouseMoveEvent(self, event: QtGui.QMouseEvent):
        if self.is_dragging:
            delta = event.globalPosition().toPoint() - self.drag_start_position
            self.move(self.pos() + delta)
            self.drag_start_position = event.globalPosition().toPoint()
 
    def mouseReleaseEvent(self, event: QtGui.QMouseEvent):
        if event.button() == QtCore.Qt.LeftButton:
            self.is_dragging = False
 
 
def configurator():
    pass  

class ESPWindow(QtWidgets.QWidget):
    def __init__(self, settings):
        super().__init__()
        self.settings = settings
        self.setWindowTitle("BY @LUXECHEATS")
        self.window_width, self.window_height = get_window_size(
            "Counter-Strike 2"
        )
        if self.window_width is None or self.window_height is None:
            print("Error: game window not found.")
            sys.exit(1)
        self.setGeometry(0, 0, self.window_width, self.window_height)
        self.setAttribute(QtCore.Qt.WA_TranslucentBackground)
        self.setAttribute(QtCore.Qt.WA_NoSystemBackground)
        self.setWindowFlags(
            QtCore.Qt.FramelessWindowHint
            | QtCore.Qt.WindowStaysOnTopHint
            | QtCore.Qt.Tool
        )
        hwnd = self.winId()
        win32gui.SetWindowLong(
            hwnd,
            win32con.GWL_EXSTYLE,
            win32con.WS_EX_LAYERED | win32con.WS_EX_TRANSPARENT,
        )
 
        self.file_watcher = QFileSystemWatcher([CONFIG_FILE])
        self.file_watcher.fileChanged.connect(self.reload_settings)
 
        self.offsets, self.client_dll = get_offsets_and_client_dll()
        self.pm = pymem.Pymem("cs2.exe")
        self.client = pymem.process.module_from_name(
            self.pm.process_handle, "client.dll"
        ).lpBaseOfDll
 
        self.scene = QGraphicsScene(self)
        self.view = QGraphicsView(self.scene, self)
        self.view.setGeometry(0, 0, self.window_width, self.window_height)
        self.view.setRenderHint(QtGui.QPainter.Antialiasing)
        self.view.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.view.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.view.setStyleSheet("background: transparent;")
        self.view.setSceneRect(0, 0, self.window_width, self.window_height)
        self.view.setFrameShape(QtWidgets.QFrame.NoFrame)
 
        self.timer = QtCore.QTimer(self)
        self.timer.timeout.connect(self.update_scene)
        self.timer.start(0)
 
        self.last_time = time.time()
        self.frame_count = 0
        self.fps = 0
 
    def reload_settings(self):
        self.settings = load_settings()
        self.window_width, self.window_height = get_window_size(
            "Counter-Strike 2"
        )
        if self.window_width is None or self.window_height is None:
            print("Error: game window not found.")
            sys.exit(1)
        self.setGeometry(0, 0, self.window_width, self.window_height)
        self.update_scene()
 
    def update_scene(self):
        if not self.is_game_window_active():
            self.scene.clear()
            return
 
        self.scene.clear()
        try:
            esp(
                self.scene,
                self.pm,
                self.client,
                self.offsets,
                self.client_dll,
                self.window_width,
                self.window_height,
                self.settings,
            )
            current_time = time.time()
            self.frame_count += 1
            if current_time - self.last_time >= 1.0:
                self.fps = self.frame_count
                self.frame_count = 0
                self.last_time = current_time
            fps_text = self.scene.addText(
                f"",
                QtGui.QFont("Noto Sans", 12, QtGui.QFont.Bold),
            )
            fps_text.setPos(5, 5)
            fps_text.setDefaultTextColor(QtGui.QColor(128, 0, 128))
        except Exception as e:
            print(f"Scene Update Error: {e}")
            QtWidgets.QApplication.quit()
 
    def is_game_window_active(self):
        hwnd = win32gui.FindWindow(None, "Counter-Strike 2")
        if hwnd:
            foreground_hwnd = win32gui.GetForegroundWindow()
            return hwnd == foreground_hwnd
        return False
 
 
def esp(
    scene,
    pm,
    client,
    offsets,
    client_dll,
    window_width,
    window_height,
    settings,
):
    if settings["esp_rendering"] == 0:
        return
 
    dwEntityList = offsets["client.dll"]["dwEntityList"]
    dwLocalPlayerPawn = offsets["client.dll"]["dwLocalPlayerPawn"]
    dwViewMatrix = offsets["client.dll"]["dwViewMatrix"]
    dwPlantedC4 = offsets["client.dll"]["dwPlantedC4"]
    m_iTeamNum = client_dll["client.dll"]["classes"]["C_BaseEntity"]["fields"][
        "m_iTeamNum"
    ]
    m_lifeState = client_dll["client.dll"]["classes"]["C_BaseEntity"]["fields"][
        "m_lifeState"
    ]
    m_pGameSceneNode = client_dll["client.dll"]["classes"]["C_BaseEntity"][
        "fields"
    ]["m_pGameSceneNode"]
    m_modelState = client_dll["client.dll"]["classes"]["CSkeletonInstance"][
        "fields"
    ]["m_modelState"]
    m_hPlayerPawn = client_dll["client.dll"]["classes"]["CCSPlayerController"][
        "fields"
    ]["m_hPlayerPawn"]
    m_iHealth = client_dll["client.dll"]["classes"]["C_BaseEntity"]["fields"][
        "m_iHealth"
    ]
    m_iszPlayerName = client_dll["client.dll"]["classes"][
        "CBasePlayerController"
    ]["fields"]["m_iszPlayerName"]
    m_pClippingWeapon = client_dll["client.dll"]["classes"][
        "C_CSPlayerPawnBase"
    ]["fields"]["m_pClippingWeapon"]
    m_AttributeManager = client_dll["client.dll"]["classes"]["C_EconEntity"][
        "fields"
    ]["m_AttributeManager"]
    m_Item = client_dll["client.dll"]["classes"]["C_AttributeContainer"][
        "fields"
    ]["m_Item"]
    m_iItemDefinitionIndex = client_dll["client.dll"]["classes"][
        "C_EconItemView"
    ]["fields"]["m_iItemDefinitionIndex"]
    m_ArmorValue = client_dll["client.dll"]["classes"]["C_CSPlayerPawn"][
        "fields"
    ]["m_ArmorValue"]
    m_vecAbsOrigin = client_dll["client.dll"]["classes"]["CGameSceneNode"][
        "fields"
    ]["m_vecAbsOrigin"]
    m_flTimerLength = client_dll["client.dll"]["classes"]["C_PlantedC4"][
        "fields"
    ]["m_flTimerLength"]
    m_flDefuseLength = client_dll["client.dll"]["classes"]["C_PlantedC4"][
        "fields"
    ]["m_flDefuseLength"]
    m_bBeingDefused = client_dll["client.dll"]["classes"]["C_PlantedC4"][
        "fields"
    ]["m_bBeingDefused"]
 
    view_matrix = [
        pm.read_float(client + dwViewMatrix + i * 4) for i in range(16)
    ]
 
    local_player_pawn_addr = pm.read_longlong(client + dwLocalPlayerPawn)
    try:
        local_player_team = pm.read_int(local_player_pawn_addr + m_iTeamNum)
    except:
        return
 
    no_center_x = window_width / 2
    no_center_y = window_height * 0.9
    entity_list = pm.read_longlong(client + dwEntityList)
    entity_ptr = pm.read_longlong(entity_list + 0x10)
 
    def bombisplant():
        global BombPlantedTime
        bombisplant = pm.read_bool(client + dwPlantedC4 - 0x8)
        if bombisplant:
            if BombPlantedTime == 0:
                BombPlantedTime = time.time()
        else:
            BombPlantedTime = 0
        return bombisplant
 
    def getC4BaseClass():
        plantedc4 = pm.read_longlong(client + dwPlantedC4)
        plantedc4class = pm.read_longlong(plantedc4)
        return plantedc4class
 
    def getPositionWTS():
        c4node = pm.read_longlong(getC4BaseClass() + m_pGameSceneNode)
        c4posX = pm.read_float(c4node + m_vecAbsOrigin)
        c4posY = pm.read_float(c4node + m_vecAbsOrigin + 0x4)
        c4posZ = pm.read_float(c4node + m_vecAbsOrigin + 0x8)
        bomb_pos = w2s(
            view_matrix, c4posX, c4posY, c4posZ, window_width, window_height
        )
        return bomb_pos
 
    def getBombTime():
        BombTime = pm.read_float(getC4BaseClass() + m_flTimerLength) - (
            time.time() - BombPlantedTime
        )
        return BombTime if (BombTime >= 0) else 0
 
    def isBeingDefused():
        global BombDefusedTime
        BombIsDefused = pm.read_bool(getC4BaseClass() + m_bBeingDefused)
        if BombIsDefused:
            if BombDefusedTime == 0:
                BombDefusedTime = time.time()
        else:
            BombDefusedTime = 0
        return BombIsDefused
 
    def getDefuseTime():
        DefuseTime = pm.read_float(getC4BaseClass() + m_flDefuseLength) - (
            time.time() - BombDefusedTime
        )
        return DefuseTime if (isBeingDefused() and DefuseTime >= 0) else 0
 
    bfont = QtGui.QFont("Noto Sans", 10, QtGui.QFont.Bold)
 
    if settings.get("bomb_esp", 0) == 1:
        if bombisplant():
            BombPosition = getPositionWTS()
            BombTime = getBombTime()
            DefuseTime = getDefuseTime()
 
            if BombPosition[0] > 0 and BombPosition[1] > 0:
                if DefuseTime > 0:
                    c4_name_text = scene.addText(
                        f"BOMB {round(BombTime, 2)} | DIF {round(DefuseTime, 2)}",
                        bfont,
                    )
                else:
                    c4_name_text = scene.addText(
                        f"EXPLODIR EM {round(BombTime, 2)}", bfont
                    )
                c4_name_x = BombPosition[0]
                c4_name_y = BombPosition[1]
                c4_name_text.setPos(c4_name_x, c4_name_y)
                c4_name_text.setDefaultTextColor(QtGui.QColor(255, 255, 255))
 
    for i in range(1, 64):
        try:
            if entity_ptr == 0:
                break
 
            entity_controller = pm.read_longlong(
                entity_ptr + 0x78 * (i & 0x1FF)
            )
            if entity_controller == 0:
                continue
 
            entity_controller_pawn = pm.read_longlong(
                entity_controller + m_hPlayerPawn
            )
            if entity_controller_pawn == 0:
                continue
 
            entity_list_pawn = pm.read_longlong(
                entity_list
                + 0x8 * ((entity_controller_pawn & 0x7FFF) >> 0x9)
                + 0x10
            )
            if entity_list_pawn == 0:
                continue
 
            entity_pawn_addr = pm.read_longlong(
                entity_list_pawn + 0x78 * (entity_controller_pawn & 0x1FF)
            )
            if (
                entity_pawn_addr == 0
                or entity_pawn_addr == local_player_pawn_addr
            ):
                continue
 
            entity_team = pm.read_int(entity_pawn_addr + m_iTeamNum)
            if entity_team == local_player_team and settings["esp_mode"] == 0:
                continue
 
            entity_hp = pm.read_int(entity_pawn_addr + m_iHealth)
            armor_hp = pm.read_int(entity_pawn_addr + m_ArmorValue)
            if entity_hp <= 0:
                continue
 
            entity_alive = pm.read_int(entity_pawn_addr + m_lifeState)
            if entity_alive != 256:
                continue
 
            weapon_pointer = pm.read_longlong(
                entity_pawn_addr + m_pClippingWeapon
            )
            weapon_index = pm.read_int(
                weapon_pointer
                + m_AttributeManager
                + m_Item
                + m_iItemDefinitionIndex
            )
            weapon_name = get_weapon_name_by_index(weapon_index)
 
            color = (
                QtGui.QColor(71, 167, 106)
                if entity_team == local_player_team
                else QtGui.QColor(196, 30, 58)
            )
            game_scene = pm.read_longlong(entity_pawn_addr + m_pGameSceneNode)
            bone_matrix = pm.read_longlong(game_scene + m_modelState + 0x80)
 
            try:
                headX = pm.read_float(bone_matrix + 6 * 0x20)
                headY = pm.read_float(bone_matrix + 6 * 0x20 + 0x4)
                headZ = pm.read_float(bone_matrix + 6 * 0x20 + 0x8) + 8
                head_pos = w2s(
                    view_matrix,
                    headX,
                    headY,
                    headZ,
                    window_width,
                    window_height,
                )
                if head_pos[1] < 0:
                    continue
                
                if settings["line_rendering"] == 1:
                    fixed_top_x = window_width // 2  # Ponto fixo centralizado no topo da tela
                    fixed_top_y = 0  # Ponto fixo no topo da tela
                    
                    line = scene.addLine(
                        fixed_top_x,
                        fixed_top_y,
                        head_pos[0],
                        head_pos[1],
                        QtGui.QPen(color, 1),
                    )

                legZ = pm.read_float(bone_matrix + 28 * 0x20 + 0x8)
                leg_pos = w2s(
                    view_matrix, headX, headY, legZ, window_width, window_height
                )
                deltaZ = abs(head_pos[1] - leg_pos[1])
                leftX = head_pos[0] - deltaZ // 4
                rightX = head_pos[0] + deltaZ // 4
                rect = scene.addRect(
                    QtCore.QRectF(
                        leftX,
                        head_pos[1],
                        rightX - leftX,
                        leg_pos[1] - head_pos[1],
                    ),
                    QtGui.QPen(color, 1),
                    QtCore.Qt.NoBrush,
                )
 
                if settings["hp_bar_rendering"] == 1:
                    max_hp = 100
                    hp_percentage = min(1.0, max(0.0, entity_hp / max_hp))
                    hp_bar_width = 4  
                    hp_bar_height = deltaZ
                    hp_bar_x_left = leftX - hp_bar_width - 2
                    hp_bar_y_top = head_pos[1]

                    # Define a cor da barra de vida com base no HP
                    if entity_hp >= 70:
                        hp_color = QtGui.QColor(0, 255, 0)  # Verde
                    elif entity_hp >= 40:
                        hp_color = QtGui.QColor(255, 255, 0)  # Amarelo
                    else:
                        hp_color = QtGui.QColor(255, 0, 0)  # Vermelho

                    current_hp_height = hp_bar_height * hp_percentage
                    hp_bar_y_bottom = (
                        hp_bar_y_top + hp_bar_height - current_hp_height
                    )

                    # Desenha o contorno preto fixo no tamanho máximo (100 HP)
                    scene.addRect(
                        QtCore.QRectF(
                            hp_bar_x_left,
                            hp_bar_y_top,
                            hp_bar_width,
                            hp_bar_height,
                        ),
                        QtGui.QPen(QtGui.QColor(0, 0, 0), 1),  # Contorno preto
                        QtGui.QBrush(QtCore.Qt.NoBrush),  # Sem fundo
                    )

                    # Desenha a barra de vida dentro do contorno fixo
                    scene.addRect(
                        QtCore.QRectF(
                            hp_bar_x_left,
                            hp_bar_y_bottom,
                            hp_bar_width,
                            current_hp_height,
                        ),
                        QtGui.QPen(QtCore.Qt.NoPen),  # Sem contorno na barra de vida
                        hp_color,  # Cor da vida
                    )
    
                if settings.get("bons", 0) == 1:
                    # Ativar a renderização do esqueleto e da hitbox
                    settings["head_hitbox_rendering"] = 1
                    
                    draw_bones(
                        scene,
                        pm,
                        bone_matrix,
                        view_matrix,
                        window_width,
                        window_height,
                    )

                if settings["head_hitbox_rendering"] == 1:
                    head_hitbox_size = (rightX - leftX) / 5
                    head_hitbox_radius = head_hitbox_size * 2**0.5 / 2
                    head_hitbox_x = leftX + 2.5 * head_hitbox_size
                    head_hitbox_y = head_pos[1] + deltaZ / 9
                    ellipse = scene.addEllipse(
                        QtCore.QRectF(
                            head_hitbox_x - head_hitbox_radius,
                            head_hitbox_y - head_hitbox_radius,
                            head_hitbox_radius * 2,
                            head_hitbox_radius * 2,
                        ),
                        QtGui.QPen(QtGui.QColor(255, 0, 0, 255)),  # Apenas contorno vermelho
                        QtGui.QBrush(QtCore.Qt.NoBrush),  # Sem preenchimento
                    )

                if settings.get("nickname", 0) == 1:
                    player_name = pm.read_string(
                        entity_controller + m_iszPlayerName, 32
                    )
                    font_size = max(6, min(18, deltaZ / 25))
                    font = QtGui.QFont("Noto Sans", font_size, QtGui.QFont.Bold)
                    name_text = scene.addText(player_name, font)
                    text_rect = name_text.boundingRect()
                    name_x = head_pos[0] - text_rect.width() / 2
                    name_y = head_pos[1] - text_rect.height()
                    name_text.setPos(name_x, name_y)
                    name_text.setDefaultTextColor(QtGui.QColor(255, 255, 255))

                if settings.get("weapon", 0) == 1:
                    weapon_name_text = scene.addText(weapon_name, font)
                    text_rect = weapon_name_text.boundingRect()
                    weapon_name_x = head_pos[0] - text_rect.width() / 2
                    weapon_name_y = head_pos[1] + deltaZ
                    weapon_name_text.setPos(weapon_name_x, weapon_name_y)
                    weapon_name_text.setDefaultTextColor(
                        QtGui.QColor(255, 255, 255)
                    )
 
                if "radius" in settings:
                    if settings["radius"] != 0:
                        center_x = window_width / 2
                        center_y = window_height / 2
                        screen_radius = (
                            settings["radius"] / 100.0 * min(center_x, center_y)
                        )
                        ellipse = scene.addEllipse(
                            QtCore.QRectF(
                                center_x - screen_radius,
                                center_y - screen_radius,
                                screen_radius * 2,
                                screen_radius * 2,
                            ),
                            QtGui.QPen(QtGui.QColor(255, 255, 255, 16), 0.5),
                            QtCore.Qt.NoBrush,
                        )
 
            except:
                return
        except:
            return
 
 
def get_weapon_name_by_index(index):
    weapon_names = {
        32: "P2000",
        61: "USP-S",
        4: "Glock",
        2: "Dual Berettas",
        36: "P250",
        30: "Tec-9",
        63: "CZ75-Auto",
        1: "Desert Eagle",
        3: "Five-SeveN",
        64: "R8",
        35: "Nova",
        25: "XM1014",
        27: "MAG-7",
        29: "Sawed-Off",
        14: "M249",
        28: "Negev",
        17: "MAC-10",
        23: "MP5-SD",
        24: "UMP-45",
        19: "P90",
        26: "Bizon",
        34: "MP9",
        33: "MP7",
        10: "FAMAS",
        16: "M4A4",
        60: "M4A1-S",
        8: "AUG",
        43: "Galil",
        7: "AK-47",
        39: "SG 553",
        40: "SSG 08",
        9: "AWP",
        38: "SCAR-20",
        11: "G3SG1",
        43: "Flashbang",
        44: "Hegrenade",
        45: "Smoke",
        46: "Molotov",
        47: "Decoy",
        48: "Incgrenage",
        49: "C4",
        31: "Taser",
        42: "Knife",
        41: "Knife Gold",
        59: "Knife",
        80: "Knife Ghost",
        500: "Knife Bayonet",
        505: "Knife Flip",
        506: "Knife Gut",
        507: "Knife Karambit",
        508: "Knife M9",
        509: "Knife Tactica",
        512: "Knife Falchion",
        514: "Knife Survival Bowie",
        515: "Knife Butterfly",
        516: "Knife Rush",
        519: "Knife Ursus",
        520: "Knife Gypsy Jackknife",
        522: "Knife Stiletto",
        523: "Knife Widowmaker",
    }
 
    return weapon_names.get(index, "Unknown")
 
 
def draw_bones(scene, pm, bone_matrix, view_matrix, width, height):
    bone_ids = {
        "neck": 5,
        "spine": 4,
        "pelvis": 0,
        "left_shoulder": 13,
        "left_elbow": 14,
        "left_wrist": 15,
        "right_shoulder": 9,
        "right_elbow": 10,
        "right_wrist": 11,
        "left_hip": 25,
        "left_knee": 26,
        "left_ankle": 27,
        "right_hip": 22,
        "right_knee": 23,
        "right_ankle": 24,
    }
    
    bone_connections = [
        ("neck", "spine"),
        ("spine", "pelvis"),
        ("pelvis", "left_hip"),
        ("left_hip", "left_knee"),
        ("left_knee", "left_ankle"),
        ("pelvis", "right_hip"),
        ("right_hip", "right_knee"),
        ("right_knee", "right_ankle"),
        ("neck", "left_shoulder"),
        ("left_shoulder", "left_elbow"),
        ("left_elbow", "left_wrist"),
        ("neck", "right_shoulder"),
        ("right_shoulder", "right_elbow"),
        ("right_elbow", "right_wrist"),
    ]

    bone_positions = {}
    try:
        for bone_name, bone_id in bone_ids.items():
            boneX = pm.read_float(bone_matrix + bone_id * 0x20)
            boneY = pm.read_float(bone_matrix + bone_id * 0x20 + 0x4)
            boneZ = pm.read_float(bone_matrix + bone_id * 0x20 + 0x8)
            bone_pos = w2s(view_matrix, boneX, boneY, boneZ, width, height)
            if bone_pos[0] != -999 and bone_pos[1] != -999:
                bone_positions[bone_name] = bone_pos

        for connection in bone_connections:
            if connection[0] in bone_positions and connection[1] in bone_positions:
                scene.addLine(
                    bone_positions[connection[0]][0],
                    bone_positions[connection[0]][1],
                    bone_positions[connection[1]][0],
                    bone_positions[connection[1]][1],
                    QtGui.QPen(QtGui.QColor(255, 0, 0, 255), 1),  # Vermelho sólido
                )

    except Exception as e:
        print(f"Error drawing bones: {e}")
 
 
def esp_main():
    settings = load_settings()
    app = QtWidgets.QApplication(sys.argv)
    window = ESPWindow(settings)
    window.show()
    sys.exit(app.exec())
 
 
def triggerbot():
    offsets = requests.get(
        "https://luxecheatsoffsets.netlify.app/luxecheatsoffsets.json"
    ).json()
    client_dll = requests.get(
        "https://luxecheatsoffsets.netlify.app/dados.json"
    ).json()
    dwEntityList = offsets["client.dll"]["dwEntityList"]
    dwLocalPlayerPawn = offsets["client.dll"]["dwLocalPlayerPawn"]
    m_iTeamNum = client_dll["client.dll"]["classes"]["C_BaseEntity"]["fields"][
        "m_iTeamNum"
    ]
    m_iIDEntIndex = client_dll["client.dll"]["classes"]["C_CSPlayerPawnBase"][
        "fields"
    ]["m_iIDEntIndex"]
    m_iHealth = client_dll["client.dll"]["classes"]["C_BaseEntity"]["fields"][
        "m_iHealth"
    ]
    mouse = Controller()
    default_settings = {
        "keyboards": "X",
        "trigger_bot_active": 1,
        "esp_mode": 1,
    }
 
    def load_settings():
        if os.path.exists(CONFIG_FILE):
            try:
                with open(CONFIG_FILE, "r") as f:
                    return json.load(f)
            except json.JSONDecodeError:
                pass
        return default_settings
 
    def main(settings):
        pm = pymem.Pymem("cs2.exe")
        client = pymem.process.module_from_name(
            pm.process_handle, "client.dll"
        ).lpBaseOfDll
        while True:
            try:
                trigger_bot_active = settings["trigger_bot_active"]
                attack_all = settings["esp_mode"]
                keyboards = settings["keyboards"]
                if win32api.GetAsyncKeyState(ord(keyboards)):
                    if trigger_bot_active == 1:
                        try:
                            player = pm.read_longlong(
                                client + dwLocalPlayerPawn
                            )
                            entityId = pm.read_int(player + m_iIDEntIndex)
                            if entityId > 0:
                                entList = pm.read_longlong(
                                    client + dwEntityList
                                )
                                entEntry = pm.read_longlong(
                                    entList + 0x8 * (entityId >> 9) + 0x10
                                )
                                entity = pm.read_longlong(
                                    entEntry + 120 * (entityId & 0x1FF)
                                )
                                entityTeam = pm.read_int(entity + m_iTeamNum)
                                playerTeam = pm.read_int(player + m_iTeamNum)
                                if (attack_all == 1) or (
                                    entityTeam != playerTeam and attack_all == 0
                                ):
                                    entityHp = pm.read_int(entity + m_iHealth)
                                    if entityHp > 0:
                                        mouse.press(Button.left)
                                        time.sleep(0.03)
                                        mouse.release(Button.left)
                        except Exception:
                            pass
                    time.sleep(0.03)
                else:
                    time.sleep(0.1)
            except KeyboardInterrupt:
                break
            except Exception:
                time.sleep(1)
 
    def start_main_thread(settings):
        while True:
            main(settings)
 
    def setup_watcher(app, settings):
        watcher = QFileSystemWatcher()
        watcher.addPath(CONFIG_FILE)
 
        def reload_settings():
            new_settings = load_settings()
            settings.update(new_settings)
 
        watcher.fileChanged.connect(reload_settings)
        app.exec()
 
    def main_program():
        app = QCoreApplication(sys.argv)
        settings = load_settings()
        threading.Thread(
            target=start_main_thread, args=(settings,), daemon=True
        ).start()
        setup_watcher(app, settings)
 
    main_program()
 
 
def aim():
    default_settings = {
        "esp_rendering": 1,
        "esp_mode": 1,
        "keyboard": "Z",
        "aim_active": 1,
        "aim_mode": 1,
        "radius": 3, # tamanho fov
        "aim_mode_distance": 1,
    }
 
    def get_window_size(window_name="Counter-Strike 2"):
        hwnd = win32gui.FindWindow(None, window_name)
        if hwnd:
            rect = win32gui.GetClientRect(hwnd)
            return rect[2] - rect[0], rect[3] - rect[1]
        return 1920, 1080
 
    def load_settings():
        if os.path.exists(CONFIG_FILE):
            try:
                with open(CONFIG_FILE, "r") as f:
                    return json.load(f)
            except json.JSONDecodeError:
                pass
        return default_settings
 
    def get_offsets_and_client_dll():
        offsets = requests.get(
            "https://luxecheatsoffsets.netlify.app/luxecheatsoffsets.json"
        ).json()
        client_dll = requests.get(
            "https://luxecheatsoffsets.netlify.app/dados.json"
        ).json()
        return offsets, client_dll
 
    def esp(
        pm, client, offsets, client_dll, settings, target_list, window_size
    ):
        width, height = window_size
        if settings["aim_active"] == 0:
            return
        dwEntityList = offsets["client.dll"]["dwEntityList"]
        dwLocalPlayerPawn = offsets["client.dll"]["dwLocalPlayerPawn"]
        dwViewMatrix = offsets["client.dll"]["dwViewMatrix"]
        m_iTeamNum = client_dll["client.dll"]["classes"]["C_BaseEntity"][
            "fields"
        ]["m_iTeamNum"]
        m_lifeState = client_dll["client.dll"]["classes"]["C_BaseEntity"][
            "fields"
        ]["m_lifeState"]
        m_pGameSceneNode = client_dll["client.dll"]["classes"]["C_BaseEntity"][
            "fields"
        ]["m_pGameSceneNode"]
        m_modelState = client_dll["client.dll"]["classes"]["CSkeletonInstance"][
            "fields"
        ]["m_modelState"]
        m_hPlayerPawn = client_dll["client.dll"]["classes"][
            "CCSPlayerController"
        ]["fields"]["m_hPlayerPawn"]
        view_matrix = [
            pm.read_float(client + dwViewMatrix + i * 4) for i in range(16)
        ]
        local_player_pawn_addr = pm.read_longlong(client + dwLocalPlayerPawn)
        try:
            local_player_team = pm.read_int(local_player_pawn_addr + m_iTeamNum)
        except:
            return
        entity_list = pm.read_longlong(client + dwEntityList)
        entity_ptr = pm.read_longlong(entity_list + 0x10)
 
        for i in range(1, 64):
            try:
                if entity_ptr == 0:
                    break
 
                entity_controller = pm.read_longlong(
                    entity_ptr + 0x78 * (i & 0x1FF)
                )
                if entity_controller == 0:
                    continue
 
                entity_controller_pawn = pm.read_longlong(
                    entity_controller + m_hPlayerPawn
                )
                if entity_controller_pawn == 0:
                    continue
 
                entity_list_pawn = pm.read_longlong(
                    entity_list
                    + 0x8 * ((entity_controller_pawn & 0x7FFF) >> 0x9)
                    + 0x10
                )
                if entity_list_pawn == 0:
                    continue
 
                entity_pawn_addr = pm.read_longlong(
                    entity_list_pawn + 0x78 * (entity_controller_pawn & 0x1FF)
                )
                if (
                    entity_pawn_addr == 0
                    or entity_pawn_addr == local_player_pawn_addr
                ):
                    continue
 
                entity_team = pm.read_int(entity_pawn_addr + m_iTeamNum)
                if (
                    entity_team == local_player_team
                    and settings["esp_mode"] == 0
                ):
                    continue
 
                entity_alive = pm.read_int(entity_pawn_addr + m_lifeState)
                if entity_alive != 256:
                    continue
                game_scene = pm.read_longlong(
                    entity_pawn_addr + m_pGameSceneNode
                )
                bone_matrix = pm.read_longlong(game_scene + m_modelState + 0x80)
                try:
                    bone_id = 6 if settings["aim_mode"] == 1 else 4
                    headX = pm.read_float(bone_matrix + bone_id * 0x20)
                    headY = pm.read_float(bone_matrix + bone_id * 0x20 + 0x4)
                    headZ = pm.read_float(bone_matrix + bone_id * 0x20 + 0x8)
                    head_pos = w2s(
                        view_matrix, headX, headY, headZ, width, height
                    )
                    legZ = pm.read_float(bone_matrix + 28 * 0x20 + 0x8)
                    leg_pos = w2s(
                        view_matrix, headX, headY, legZ, width, height
                    )
                    deltaZ = abs(head_pos[1] - leg_pos[1])
                    if head_pos[0] != -999 and head_pos[1] != -999:
                        if settings["aim_mode_distance"] == 1:
                            target_list.append(
                                {"pos": head_pos, "deltaZ": deltaZ}
                            )
                        else:
                            target_list.append(
                                {"pos": head_pos, "deltaZ": None}
                            )
                except Exception as e:
                    pass
            except:
                return
        return target_list
 
    def aimbot(target_list, radius, aim_mode_distance):
        if not target_list:
            return
        center_x = win32api.GetSystemMetrics(0) // 2
        center_y = win32api.GetSystemMetrics(1) // 2
        if radius == 0:
            closest_target = None
            closest_dist = float("inf")
            for target in target_list:
                dist = (
                    (target["pos"][0] - center_x) ** 2
                    + (target["pos"][1] - center_y) ** 2
                ) ** 0.5
                if dist < closest_dist:
                    closest_target = target["pos"]
                    closest_dist = dist
        else:
            screen_radius = radius / 100.0 * min(center_x, center_y)
            closest_target = None
            closest_dist = float("inf")
            if aim_mode_distance == 1:
                target_with_max_deltaZ = None
                max_deltaZ = -float("inf")
                for target in target_list:
                    dist = (
                        (target["pos"][0] - center_x) ** 2
                        + (target["pos"][1] - center_y) ** 2
                    ) ** 0.5
                    if dist < screen_radius and target["deltaZ"] > max_deltaZ:
                        max_deltaZ = target["deltaZ"]
                        target_with_max_deltaZ = target
                closest_target = (
                    target_with_max_deltaZ["pos"]
                    if target_with_max_deltaZ
                    else None
                )
            else:
                for target in target_list:
                    dist = (
                        (target["pos"][0] - center_x) ** 2
                        + (target["pos"][1] - center_y) ** 2
                    ) ** 0.5
                    if dist < screen_radius and dist < closest_dist:
                        closest_target = target["pos"]
                        closest_dist = dist
        if closest_target:
            target_x, target_y = closest_target
            win32api.mouse_event(
                win32con.MOUSEEVENTF_MOVE,
                int(target_x - center_x),
                int(target_y - center_y),
                0,
                0,
            )
 
    def main(settings):
        offsets, client_dll = get_offsets_and_client_dll()
        window_size = get_window_size()
        pm = pymem.Pymem("cs2.exe")
        client = pymem.process.module_from_name(
            pm.process_handle, "client.dll"
        ).lpBaseOfDll
        while True:
            target_list = []
            target_list = esp(
                pm,
                client,
                offsets,
                client_dll,
                settings,
                target_list,
                window_size,
            )
            if win32api.GetAsyncKeyState(ord(settings["keyboard"])):
                aimbot(
                    target_list,
                    settings["radius"],
                    settings["aim_mode_distance"],
                )
            time.sleep(0.001)
 
    def start_main_thread(settings):
        while True:
            main(settings)
 
    def setup_watcher(app, settings):
        watcher = QFileSystemWatcher()
        watcher.addPath(CONFIG_FILE)
 
        def reload_settings():
            new_settings = load_settings()
            settings.update(new_settings)
 
        watcher.fileChanged.connect(reload_settings)
        app.exec()
 
    def main_program():
        app = QCoreApplication(sys.argv)
        settings = load_settings()
        threading.Thread(
            target=start_main_thread, args=(settings,), daemon=True
        ).start()
        setup_watcher(app, settings)
 
    main_program()
 
 
if __name__ == "__main__":
    print("ABRA O JOGO PRIMEIRO.")
    while True:
        time.sleep(1)
        try:
            pm = pymem.Pymem("cs2.exe")
            client = pymem.process.module_from_name(
                pm.process_handle, "client.dll"
            ).lpBaseOfDll
            break
        except Exception as e:
            pass
    print("INJETANDO CHEAT.")
    time.sleep(2)
 
    process1 = multiprocessing.Process(target=configurator)
    process2 = multiprocessing.Process(target=esp_main)
    process3 = multiprocessing.Process(target=triggerbot)
    process4 = multiprocessing.Process(target=aim)
 
    process1.start()
    process2.start()
    process3.start()
    process4.start()
 
    process1.join()
    process2.join()
    process3.join()
    process4.join()
