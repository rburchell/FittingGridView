import FittingGridView 1.0
import QtQuick 2.6

FittingGridView {
    height: 600
    width: 800
    model: ListModel {
        ListElement {
            url: "cheetah-sunset.jpg"
        }
        ListElement {
            url: "firewatch-game-graphics.jpg"
        }
        ListElement {
            url: "high-definition-mobile-phone-wallpapers-720x1280-hd-pink-aura.jpg"
        }
        ListElement {
            url: "hd-iphone-4s-wallpaper-1.jpg"
        }
        ListElement {
            url: "ice.jpg"
        }
        ListElement {
            url: "high-definition-mobile-phone-wallpapers-720x1280-hd-paint-colours.jpg"
        }
        ListElement {
            url: "quad-hd-mobile-phone-wallpapers-1440x2560-neon-swirls.jpg"
        }
        ListElement {
            url: "10491-pink-floyd-6.jpg"
        }
    }
    delegate: Image {
        source: "img/" + model.url
    }
}
