Vagrant.configure("2") do |config|
    config.vm.box = "ubuntu/trusty64"
    config.vm.provision :shell, path: "bootstrap.sh"
    config.vm.network "private_network", ip: "192.168.4.2"
end
