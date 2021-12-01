# esphome custom component(s) 

This repository contains a collection of custom components
for [ESPHome](https://esphome.io/).  Currently only one! ;)

## 1. Usage

Use latest [ESPHome](https://esphome.io/) (at least v1.18.0)
with external components and add this to your `.yaml` definition:

```yaml
external_components:
  - source: github:///tiaanv/esphome-components
```

## 2. Components

### 2.1. `t547`

A Display component to support the [LILYGO T5 4.7" E-paper display](http://www.lilygo.cn/prod_view.aspx?TypeId=50061&Id=1384&FId=t3:50061:3).
For more info in the display components, see the [ESP Home Display Documentation](https://esphome.io/#display-components)

#### 2.1.1. Example

```yaml
# The Basic Display Definition in ESPhome

display:
- platform: t547
  id: t5_display
  update_interval: 30s



