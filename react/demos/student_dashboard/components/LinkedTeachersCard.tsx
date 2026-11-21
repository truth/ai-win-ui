import { createElement } from "../../../runtime/index.ts";
import { Panel, Label, Icon, Image } from "../../../runtime/index.ts";
import { TEACHERS } from "../data/mockData.ts";

export interface LinkedTeachersCardProps {
  onSeeAll?: () => void;
  onContactTeacher?: (teacherId: string, action: "chat" | "phone") => void;
}

export function LinkedTeachersCard({
  onSeeAll,
  onContactTeacher,
}: LinkedTeachersCardProps) {
  return (
    <Panel direction="column" padding={0} spacing={8} alignItems="stretch">
      {/* External Title Row */}
      <Panel
        direction="row"
        alignItems="center"
        justifyContent="space-between"
        height={22}
        padding={0}
      >
        <Label text="Linked Teachers" fontSize={13} bold={true} color="#1E2538" flexGrow={1} />
        <Panel direction="row" alignItems="center" justifyContent="end" width={46} minWidth={46} height={22} padding={0} onClick={onSeeAll}>
          <Label text="See all" fontSize={11} fontWeight={500} color="#3D509B" />
        </Panel>
      </Panel>

      {/* Separate White Card per Teacher */}
      <Panel direction="column" spacing={8} padding={0} alignItems="stretch">
        {TEACHERS.map((t) => (
          <Panel
            key={t.id}
            direction="row"
            alignItems="center"
            justifyContent="space-between"
            height={52}
            padding={{ left: 12, top: 6, right: 14, bottom: 6 }}
            cornerRadius={14}
            background="#FFFFFF"
          >
            {/* Left: Avatar + Name + Subject */}
            <Panel direction="row" alignItems="center" flexGrow={1} flexBasis={0} spacing={10} padding={0}>
              <Image
                src={t.avatar}
                width={32}
                height={32}
                cornerRadius={16}
                stretch="uniformToFill"
              />
              <Panel direction="column" flexGrow={1} flexBasis={0} padding={0} spacing={2}>
                <Label
                  text={t.role ? `${t.name} ${t.role}` : t.name}
                  fontSize={11.5}
                  bold={true}
                  color="#1E2538"
                />
                <Label text={t.subject} fontSize={9.5} color="#64748B" />
              </Panel>
            </Panel>

            {/* Right: Mail + Phone Action Icons */}
            <Panel direction="row" width={60} minWidth={60} alignItems="center" justifyContent="end" spacing={10} padding={0}>
              <Panel
                direction="row"
                alignItems="center"
                justifyContent="center"
                width={24}
                height={24}
                cornerRadius={12}
                padding={0}
                onClick={() => onContactTeacher?.(t.id, "chat")}
              >
                <Icon name="mail" size={13} color="#1E2538" strokeWidth={1.9} />
              </Panel>
              <Panel
                direction="row"
                alignItems="center"
                justifyContent="center"
                width={24}
                height={24}
                cornerRadius={12}
                padding={0}
                onClick={() => onContactTeacher?.(t.id, "phone")}
              >
                <Icon name="phone" size={13} color="#1E2538" strokeWidth={1.9} />
              </Panel>
            </Panel>
          </Panel>
        ))}
      </Panel>
    </Panel>
  );
}
