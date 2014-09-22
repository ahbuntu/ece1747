ece1747
===

Intro:
---
- http://www.eecg.toronto.edu/~amza/ece1747h/homeworks/index.html


To get started:
---

```
git clone git@github.com:dfcarney/ece1747.git
cd ece1747
cp Vagrantfile.sample Vagrantfile
vagrant up
vagrant ssh
cd /vagrant
sudo apt-get update
sudo apt-get install build-essential git-core libsdl1.2-dev libsdl-net1.2-dev libgl1-mesa-dev vim --yes

# for LaTeX; this is a *big* install
sudo apt-get install texlive-full --yes

cd assignment-1/game_benchmark_1747

make
```
