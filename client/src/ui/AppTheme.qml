import QtQuick
import Messenger 1.0

Item {
    id: themeRoot

    property string rawAccent: AppState.accentColor
    property string accent: {
        let c = Qt.color(AppState.accentColor)
        if (c.hslLightness > 0.62) {
            return Qt.hsla(c.hslHue, c.hslSaturation, 0.62, 1.0).toString()
        }
        return AppState.accentColor
    }

    property string bgMain: {
        if (AppState.theme === "night") return "#000000"
        if (AppState.theme === "day") return "#f1f2f5"
        if (AppState.theme === "tinted") return "#17212b"
        return "#0e1621"
    }

    // Фон панелей
    property string bgPanel: {
        if (AppState.theme === "night") return "#1c242f"
        if (AppState.theme === "day") return "#ffffff"
        if (AppState.theme === "tinted") return "#242f3d"
        return "#17212b"
    }

    // Фон для полей ввода
    property string bgInput: {
        if (AppState.theme === "day") return "#e4e6eb"
        if (AppState.theme === "night") return "#131a23"
        if (AppState.theme === "tinted") return "#17212b"
        return "#242f3d"
    }

    // Фон для шапок
    property string bgHeader: {
        if (AppState.theme === "day") return "#ffffff"
        if (AppState.theme === "night") return "#1c242f"
        if (AppState.theme === "tinted") return "#242f3d"
        return "#17212b"
    }

    // Основной цвет текста
    property string textMain: {
        if (AppState.theme === "day") return "#000000"
        return "#ffffff" 
    }

    // Цвет вторичного текста
    property string textHint: "#8a96a3"

    // Цвет выделенного элемента
    property string bgActiveItem: {
        if (AppState.theme === "day") return accent
        return "#2b5278" 
    }
    
    // Универсальный цвет наведения
    property string hoverColor: Qt.alpha(textMain, 0.05)

    // Цвета сообщений
    property string myBubble: {
        if (AppState.theme === "day") return accent
        return "#2b5278" 
    }
    property string otherBubble: {
        if (AppState.theme === "day") return "#ffffff"
        if (AppState.theme === "night") return "#1c242f"
        return "#18222d" 
    }
}
