import { createElement } from "../../../runtime/index.ts";
import { Panel, Label, Icon } from "../../../runtime/index.ts";
import { NAV_TABS } from "../data/mockData.ts";

export interface SidebarProps {
  activeTab: string;
  onSelectTab: (tabId: string) => void;
  onLogOut?: () => void;
}

const RAIL_LEFT_BG = "#5F74E6";
const RAIL_RIGHT_BG = "#3D509B";
const CANVAS_BG = "#F1F3F9";

export function Sidebar({ activeTab, onSelectTab, onLogOut }: SidebarProps) {
  return (
    <Panel
      direction="row"
      width={184}
      minWidth={184}
      maxWidth={184}
      padding={0}
      spacing={0}
      alignItems="stretch"
      background={RAIL_RIGHT_BG}
    >
      {/* Left Light-Blue Icon Rail */}
      <Panel
        direction="column"
        width={42}
        minWidth={42}
        maxWidth={42}
        background={RAIL_LEFT_BG}
        padding={{ bottom: 16 }}
        spacing={0}
        alignItems="stretch"
      >
        <Panel height={60} padding={0} hitTest="caption" background={RAIL_LEFT_BG} />
        {NAV_TABS.map((tab) => {
          const isActive = tab.id === activeTab;
          return (
            <Panel
              key={tab.id}
              direction="row"
              height={50}
              padding={0}
              spacing={0}
              alignItems="center"
              background={RAIL_LEFT_BG}
              onClick={() => onSelectTab(tab.id)}
            >
              {/* Left 3px White Bar on Active Tab */}
              <Panel
                width={3}
                height={20}
                cornerRadius={1.5}
                padding={0}
                background={isActive ? "#FFFFFF" : RAIL_LEFT_BG}
              />
              <Panel
                direction="row"
                flexGrow={1}
                height={50}
                padding={{ right: 3 }}
                alignItems="center"
                justifyContent="center"
              >
                <Icon
                  name={tab.icon}
                  size={15}
                  color="#FFFFFF"
                  strokeWidth={1.8}
                />
              </Panel>
            </Panel>
          );
        })}
      </Panel>

      {/* Right Dark-Navy Label Rail */}
      <Panel
        direction="column"
        flexGrow={1}
        flexBasis={0}
        background={RAIL_RIGHT_BG}
        padding={0}
        spacing={0}
        justifyContent="space-between"
        alignItems="stretch"
      >
        <Panel direction="column" padding={0} spacing={0} alignItems="stretch">
          {/* Smart Header Logo */}
          <Panel
            direction="row"
            height={60}
            alignItems="center"
            justifyContent="center"
            padding={{ top: 4 }}
            spacing={7}
            hitTest="caption"
          >
            <Icon name="graduation-cap" size={16} color="#FFFFFF" strokeWidth={2.0} />
            <Label text="Smart" fontSize={15} bold={true} color="#FFFFFF" />
          </Panel>

          {/* Navigation Labels with Seamless Concave Cutout */}
          <Panel direction="column" padding={0} spacing={0} alignItems="stretch">
            {NAV_TABS.map((tab) => {
              const isActive = tab.id === activeTab;
              if (!isActive) {
                return (
                  <Panel
                    key={tab.id}
                    direction="row"
                    height={50}
                    padding={{ left: 20, right: 8 }}
                    spacing={0}
                    alignItems="center"
                    background={RAIL_RIGHT_BG}
                    onClick={() => onSelectTab(tab.id)}
                  >
                    <Label
                      text={tab.label}
                      fontSize={12}
                      fontWeight={500}
                      color="#D9E1F7"
                    />
                  </Panel>
                );
              }
              return (
                <Panel
                  key={tab.id}
                  direction="column"
                  height={50}
                  padding={0}
                  spacing={0}
                  alignItems="stretch"
                  background={RAIL_RIGHT_BG}
                  onClick={() => onSelectTab(tab.id)}
                >
                  {/* Top Concave Curve transitioning #3D509B -> #F1F3F9 */}
                  <Panel
                    direction="row"
                    height={9}
                    padding={0}
                    spacing={0}
                    justifyContent="end"
                    alignItems="end"
                    background={RAIL_RIGHT_BG}
                  >
                    <Icon name="cutout-corner-top" size={9} color={CANVAS_BG} />
                  </Panel>

                  {/* Active Label Pill: Rounded Left + Flush Right into Main Canvas */}
                  <Panel
                    direction="row"
                    height={32}
                    padding={0}
                    spacing={0}
                    alignItems="center"
                    background={RAIL_RIGHT_BG}
                  >
                    {/* Left Rounded Half-Cap */}
                    <Panel
                      width={24}
                      height={32}
                      cornerRadius={16}
                      padding={0}
                      background={CANVAS_BG}
                    />
                    {/* Flush Right Extension + Label */}
                    <Panel
                      direction="row"
                      flexGrow={1}
                      height={32}
                      margin={{ left: -14 }}
                      padding={{ left: 6, right: 6 }}
                      alignItems="center"
                      background={CANVAS_BG}
                    >
                      <Label
                        text={tab.label}
                        fontSize={12}
                        bold={true}
                        color={RAIL_RIGHT_BG}
                      />
                    </Panel>
                  </Panel>

                  {/* Bottom Concave Curve transitioning #F1F3F9 -> #3D509B */}
                  <Panel
                    direction="row"
                    height={9}
                    padding={0}
                    spacing={0}
                    justifyContent="end"
                    alignItems="start"
                    background={RAIL_RIGHT_BG}
                  >
                    <Icon name="cutout-corner-bottom" size={9} color={CANVAS_BG} />
                  </Panel>
                </Panel>
              );
            })}
          </Panel>
        </Panel>

        {/* Bottom Log Out */}
        <Panel
          direction="row"
          height={46}
          alignItems="center"
          justifyContent="center"
          padding={{ bottom: 10 }}
          spacing={8}
          onClick={onLogOut}
        >
          <Icon name="log-out" size={14} color="#DCE3F7" />
          <Label text="Log Out" fontSize={12} fontWeight={500} color="#DCE3F7" />
        </Panel>
      </Panel>
    </Panel>
  );
}
