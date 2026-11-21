import { createElement, windowAction } from "../../../runtime/index.ts";
import { Panel, Label, Icon, TextInput, Image, Divider } from "../../../runtime/index.ts";

export interface TopBarProps {
  searchText: string;
  onSearchChange: (text: string) => void;
  language: string;
  onToggleLanguage: () => void;
  onNotifyClick?: () => void;
  onMailClick?: () => void;
}

export function TopBar({
  searchText,
  onSearchChange,
  language,
  onToggleLanguage,
  onNotifyClick,
  onMailClick,
}: TopBarProps) {
  return (
    <Panel direction="column" padding={0} spacing={6} alignItems="stretch">
      <Panel
        direction="row"
        alignItems="center"
        justifyContent="space-between"
        height={36}
        padding={0}
        spacing={10}
        hitTest="caption"
      >
        {/* Left: Pure White Pill Search Box */}
        <Panel
          direction="row"
          alignItems="center"
          width={220}
          height={30}
          background="#FFFFFF"
          borderColor="#E5E9F2"
          border={{ left: 1, top: 1, right: 1, bottom: 1 }}
          cornerRadius={15}
          padding={{ left: 12, top: 0, right: 10, bottom: 0 }}
          spacing={6}
          hitTest="client"
        >
          <TextInput
            flexGrow={1}
            flexBasis={0}
            height={24}
            placeholder="Search"
            placeholderColor="#94A3B8"
            background="#FFFFFF"
            borderColor="#FFFFFF"
            focusBorderColor="#FFFFFF"
            color="#1E293B"
            fontSize={11}
            cornerRadius={8}
            text={searchText}
            onTextChanged={onSearchChange}
          />
          <Icon name="search" size={13} color="#475569" strokeWidth={1.8} />
        </Panel>

        {/* Right: Language + Mail + Bell + User Profile + Window Controls */}
        <Panel
          direction="row"
          width={384}
          minWidth={384}
          alignItems="center"
          justifyContent="end"
          spacing={10}
          padding={0}
          hitTest="caption"
        >
          {/* Language Switcher */}
          <Panel
            direction="row"
            alignItems="center"
            justifyContent="center"
            width={52}
            height={26}
            padding={{ left: 6, right: 6 }}
            cornerRadius={13}
            spacing={4}
            hitTest="client"
            onClick={onToggleLanguage}
          >
            <Label text={language} fontSize={11} fontWeight={600} color="#334155" />
            <Icon name="chevron-down" size={11} color="#64748B" />
          </Panel>

          {/* Mail Icon */}
          <Panel
            direction="row"
            alignItems="center"
            justifyContent="center"
            width={26}
            height={26}
            cornerRadius={13}
            padding={0}
            hitTest="client"
            onClick={onMailClick}
          >
            <Icon name="mail" size={15} color="#1E2538" strokeWidth={1.8} />
          </Panel>

          {/* Notification Bell with Dot Badge */}
          <Panel
            direction="row"
            alignItems="center"
            justifyContent="center"
            width={28}
            height={26}
            cornerRadius={13}
            padding={0}
            spacing={0}
            hitTest="client"
            onClick={onNotifyClick}
          >
            <Icon name="bell" size={15} color="#1E2538" strokeWidth={1.8} />
            <Panel
              width={6}
              height={6}
              cornerRadius={3}
              background="#5F74E6"
              padding={0}
              margin={{ left: -4, bottom: 10 }}
            />
          </Panel>

          {/* Grace Stanley Profile */}
          <Panel
            direction="row"
            alignItems="center"
            width={130}
            height={30}
            spacing={7}
            padding={{ left: 2, right: 2 }}
            hitTest="client"
          >
            <Image
              src="images/avatar_grace.png"
              width={26}
              height={26}
              cornerRadius={13}
              stretch="uniformToFill"
            />
            <Label text="Grace Stanley" fontSize={11} fontWeight={600} color="#1E2538" />
            <Icon name="chevron-down" size={11} color="#64748B" />
          </Panel>

          {/* Subtle Light-Mode Frameless Window Controls */}
          <Panel
            direction="row"
            width={86}
            minWidth={86}
            height={26}
            alignItems="center"
            justifyContent="end"
            spacing={4}
            padding={{ left: 6 }}
            hitTest="client"
          >
            <Panel
              direction="row"
              alignItems="center"
              justifyContent="center"
              width={24}
              height={24}
              cornerRadius={12}
              padding={0}
              background="#E8ECF5"
              hitTest="client"
              onClick={() => windowAction("minimize")}
            >
              <Icon name="minus" size={10} color="#475569" strokeWidth={1.8} />
            </Panel>
            <Panel
              direction="row"
              alignItems="center"
              justifyContent="center"
              width={24}
              height={24}
              cornerRadius={12}
              padding={0}
              background="#E8ECF5"
              hitTest="client"
              onClick={() => windowAction("maximize")}
            >
              <Icon name="square" size={9} color="#475569" strokeWidth={1.7} />
            </Panel>
            <Panel
              direction="row"
              alignItems="center"
              justifyContent="center"
              width={24}
              height={24}
              cornerRadius={12}
              padding={0}
              background="#FCE8E8"
              hitTest="client"
              onClick={() => windowAction("close")}
            >
              <Icon name="x" size={10} color="#DC2626" strokeWidth={1.9} />
            </Panel>
          </Panel>
        </Panel>
      </Panel>

      {/* Bottom Hairline Divider */}
      <Divider color="#E0E5F0" thickness={1} />
    </Panel>
  );
}
