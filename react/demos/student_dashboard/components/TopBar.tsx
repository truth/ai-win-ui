import { createElement } from "../../../runtime/index.ts";
import { Panel, Label, Icon, TextInput, Image } from "../../../runtime/index.ts";

export interface TopBarProps {
  searchText: string;
  onSearchChange: (text: string) => void;
  language: string;
  onToggleLanguage: () => void;
  unreadCount?: number;
}

export function TopBar({
  searchText,
  onSearchChange,
  language,
  onToggleLanguage,
  unreadCount = 3,
}: TopBarProps) {
  return (
    <Panel
      direction="row"
      alignItems="center"
      justifyContent="space-between"
      height={58}
      padding={[0, 10, 0, 8]}
    >
      {/* Search Input Box */}
      <Panel
        direction="row"
        alignItems="center"
        width={300}
        height={38}
        background="#F8FAFC"
        borderColor="#E2E8F0"
        border={[1, 1, 1, 1]}
        cornerRadius={19}
        padding={[14, 0, 14, 0]}
      >
        <TextInput
          flexGrow={1}
          placeholder="Search"
          placeholderColor="#94A3B8"
          color="#1E293B"
          fontSize={13}
          text={searchText}
          onTextChanged={onSearchChange}
        />
        <Icon name="search" size={16} color="#64748B" />
      </Panel>

      {/* Right Controls */}
      <Panel direction="row" alignItems="center" spacing={18} padding={0}>
        {/* Language Picker */}
        <Panel
          direction="row"
          alignItems="center"
          padding={[12, 6, 12, 6]}
          cornerRadius={14}
          background="#F8FAFC"
          borderColor="#E2E8F0"
          border={[1, 1, 1, 1]}
          spacing={4}
          onClick={onToggleLanguage}
        >
          <Label text={language} fontSize={12} bold={true} color="#475569" />
          <Icon name="chevron-down" size={13} color="#64748B" />
        </Panel>

        {/* Mail Icon */}
        <Panel
          direction="row"
          alignItems="center"
          justifyContent="center"
          width={36}
          height={36}
          cornerRadius={18}
          background="#F8FAFC"
          borderColor="#E2E8F0"
          border={[1, 1, 1, 1]}
        >
          <Icon name="mail" size={17} color="#1E293B" />
        </Panel>

        {/* Bell / Notification Icon with Badge */}
        <Panel
          direction="row"
          alignItems="center"
          justifyContent="center"
          width={36}
          height={36}
          cornerRadius={18}
          background="#F8FAFC"
          borderColor="#E2E8F0"
          border={[1, 1, 1, 1]}
        >
          <Icon name="bell" size={17} color="#1E293B" />
        </Panel>

        {/* User Profile Info */}
        <Panel
          direction="row"
          alignItems="center"
          spacing={8}
          padding={[6, 4, 10, 4]}
          cornerRadius={20}
          background="#F8FAFC"
        >
          <Image
            src="images/avatar_grace.png"
            width={30}
            height={30}
            cornerRadius={15}
            stretch="uniform"
          />
          <Label text="Grace Stanley" fontSize={13} bold={true} color="#1E293B" />
          <Icon name="chevron-down" size={14} color="#64748B" />
        </Panel>
      </Panel>
    </Panel>
  );
}

