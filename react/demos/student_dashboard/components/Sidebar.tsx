import { createElement, Fragment } from "../../../runtime/index.ts";
import { Panel, Label, Icon, Button } from "../../../runtime/index.ts";
import { NAV_TABS, type NavTab } from "../data/mockData.ts";

export interface SidebarProps {
  activeTab: string;
  onSelectTab: (tabId: string) => void;
  onLogOut?: () => void;
}

export function Sidebar({ activeTab, onSelectTab, onLogOut }: SidebarProps) {
  return (
    <Panel
      direction="column"
      width={195}
      minWidth={195}
      maxWidth={195}
      background="#3751FE"
      padding={[0, 24, 0, 24]}
      alignItems="stretch"
      justifyContent="space-between"
    >
      {/* Top section: Logo + Nav Items */}
      <Panel direction="column" alignItems="stretch" padding={0}>
        {/* Logo Header */}
        <Panel
          direction="row"
          alignItems="center"
          padding={[24, 4, 16, 28]}
          spacing={10}
        >
          <Icon name="graduation-cap" size={20} color="#FFFFFF" />
          <Label text="Smart" fontSize={20} bold={true} color="#FFFFFF" />
        </Panel>

        {/* Navigation Items */}
        <Panel direction="column" alignItems="stretch" padding={[16, 0, 0, 0]} spacing={4}>
          {NAV_TABS.map((tab) => {
            const isActive = tab.id === activeTab;
            return (
              <Panel
                key={tab.id}
                direction="row"
                alignItems="center"
                height={46}
                padding={[0, 0, 0, 18]}
                cornerRadius={isActive ? 22 : 12}
                background={isActive ? "#FFFFFF" : "#3751FE"}
                margin={isActive ? [12, 2, 0, 2] : [12, 2, 12, 2]}
                onClick={() => onSelectTab(tab.id)}
              >
                <Icon
                  name={tab.icon}
                  size={18}
                  color={isActive ? "#3751FE" : "#D7DFFC"}
                  strokeWidth={2.0}
                />
                <Panel width={12} height={1} padding={0} />
                <Label
                  text={tab.label}
                  fontSize={14}
                  fontWeight={isActive ? 700 : 500}
                  bold={isActive}
                  color={isActive ? "#1E293B" : "#E2E8F0"}
                />
              </Panel>
            );
          })}
        </Panel>
      </Panel>

      {/* Bottom section: Log Out */}
      <Panel
        direction="row"
        alignItems="center"
        height={48}
        padding={[20, 0, 0, 24]}
        margin={[0, 0, 0, 16]}
        onClick={onLogOut}
      >
        <Icon name="log-out" size={18} color="#FFFFFF" />
        <Panel width={12} height={1} padding={0} />
        <Label text="Log Out" fontSize={14} bold={true} color="#FFFFFF" />
      </Panel>
    </Panel>
  );
}

