import { createElement } from "../../../runtime/index.ts";
import { Panel, Label, Icon, Image } from "../../../runtime/index.ts";
import { TEACHERS, type Teacher } from "../data/mockData.ts";

export interface LinkedTeachersCardProps {
  onSeeAll?: () => void;
  onContactTeacher?: (teacherId: string, action: "chat" | "phone") => void;
}

export function LinkedTeachersCard({
  onSeeAll,
  onContactTeacher,
}: LinkedTeachersCardProps) {
  return (
    <Panel
      direction="column"
      background="#FFFFFF"
      cornerRadius={18}
      padding={[18, 14, 18, 14]}
      borderColor="#E2E8F0"
      border={[1, 1, 1, 1]}
      flexGrow={1}
    >
      {/* Title & See All */}
      <Panel
        direction="row"
        alignItems="center"
        justifyContent="space-between"
        padding={0}
      >
        <Label text="Linked Teachers" fontSize={15} bold={true} color="#0F172A" />
        <Panel padding={0} onClick={onSeeAll}>
          <Label text="See all" fontSize={11} bold={true} color="#3751FE" />
        </Panel>
      </Panel>

      <Panel height={12} padding={0} />

      {/* Teachers List */}
      <Panel direction="column" spacing={10} padding={0}>
        {TEACHERS.map((t) => (
          <Panel
            key={t.id}
            direction="row"
            alignItems="center"
            justifyContent="space-between"
            padding={[10, 8, 10, 8]}
            cornerRadius={12}
            background="#F8FAFC"
          >
            {/* Left: Avatar + Details */}
            <Panel direction="row" alignItems="center" spacing={10} padding={0}>
              <Image
                src={t.avatar}
                width={30}
                height={30}
                cornerRadius={15}
                stretch="uniform"
              />
              <Panel direction="column" padding={0}>
                <Panel direction="row" alignItems="baseline" spacing={4} padding={0}>
                  <Label text={t.name} fontSize={12} bold={true} color="#0F172A" />
                  {t.role ? (
                    <Label text={t.role} fontSize={11} color="#64748B" />
                  ) : null}
                </Panel>
                <Label text={t.subject} fontSize={10} color="#94A3B8" />
              </Panel>
            </Panel>

            {/* Right Action Icons: Mail & Phone */}
            <Panel direction="row" alignItems="center" spacing={10} padding={0}>
              <Panel
                direction="row"
                alignItems="center"
                justifyContent="center"
                width={26}
                height={26}
                cornerRadius={13}
                background="#FFFFFF"
                borderColor="#E2E8F0"
                border={[1, 1, 1, 1]}
                onClick={() => onContactTeacher?.(t.id, "chat")}
              >
                <Icon name="mail" size={13} color="#0F172A" />
              </Panel>
              <Panel
                direction="row"
                alignItems="center"
                justifyContent="center"
                width={26}
                height={26}
                cornerRadius={13}
                background="#FFFFFF"
                borderColor="#E2E8F0"
                border={[1, 1, 1, 1]}
                onClick={() => onContactTeacher?.(t.id, "phone")}
              >
                <Icon name="phone" size={13} color="#0F172A" />
              </Panel>
            </Panel>
          </Panel>
        ))}
      </Panel>
    </Panel>
  );
}

