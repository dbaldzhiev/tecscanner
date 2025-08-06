
if [ "$usb_missing" = true ] || [ ! -f /usr/local/sbin/usb-automount.sh ] || [ ! -f /etc/udev/rules.d/99-usb-automount.rules ]; then
  echo "Installing USB auto-mount support..."
  "$PROJECT_ROOT/scripts/install_usbmount.sh"
else
  echo "USB auto-mount support already installed."
fi

CON_NAME="Wired connection 1"
if ! nmcli -g NAME con show | grep -Fx "$CON_NAME" >/dev/null 2>&1; then
  echo "Creating NetworkManager connection $CON_NAME..."
  sudo nmcli con add type ethernet ifname eth0 con-name "$CON_NAME"
fi
echo "Configuring static IP 192.168.6.1 on $CON_NAME..."
sudo nmcli con mod "$CON_NAME" ipv4.method manual ipv4.addresses 192.168.6.1/24
sudo nmcli con mod "$CON_NAME" ipv4.gateway ""
sudo nmcli con mod "$CON_NAME" ipv4.dns ""
sudo nmcli con mod "$CON_NAME" connection.autoconnect yes
sudo nmcli con mod "$CON_NAME" connection.interface-name eth0
sudo nmcli con up "$CON_NAME"

if [ ! -f /etc/systemd/system/tecscanner.service ]; then
  echo "Setting up Tecscanner systemd service..."
  "$PROJECT_ROOT/scripts/setup_service.sh"
else
  echo "Tecscanner systemd service already installed."
fi
