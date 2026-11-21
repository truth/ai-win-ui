import { createElement } from "../../../runtime/index.ts";
import { Panel, Label, Icon, CircularProgress } from "../../../runtime/index.ts";
import type { VisitStat } from "../data/mockData.ts";

export interface MyVisitCardProps {
  data: VisitStat[];
  selectedMonth: string;
  onToggleMonth: () => void;
}

export function MyVisitCard({
  data,
  selectedMonth,
  onToggleMonth,
}: MyVisitCardProps) {
  return (
    <Panel
      direction="column"
      width={186}
      minWidth={186}
      maxWidth={186}
      padding={0}
      spacing={8}
      alignItems="stretch"
    >
      {/* External Title Row */}
      <Panel
        direction="row"
        alignItems="center"
        justifyContent="space-between"
        height={22}
        padding={0}
      >
        <Label text="My visit" fontSize={13} bold={true} color="#1E2538" flexGrow={1} />
        <Panel
          direction="row"
          alignItems="center"
          justifyContent="center"
          width={84}
          minWidth={84}
          height={22}
          padding={{ left: 8, right: 6 }}
          cornerRadius={11}
          background="#DCE2F2"
          spacing={4}
          onClick={onToggleMonth}
        >
          <Label text={selectedMonth} fontSize={10} fontWeight={500} color="#3D509B" />
          <Icon name="chevron-down" size={10} color="#3D509B" />
        </Panel>
      </Panel>

      {/* White Card Body (expands full bottom-column height) */}
      <Panel
        direction="column"
        flexGrow={1}
        flexBasis={0}
        background="#FFFFFF"
        cornerRadius={14}
        padding={{ left: 10, top: 22, right: 10, bottom: 22 }}
        justifyContent="space-between"
        alignItems="stretch"
      >
        {[0, 2, 4].map((rowStart) => (
          <Panel
            key={rowStart}
            direction="row"
            justifyContent="space-between"
            padding={0}
            spacing={6}
          >
            {data.slice(rowStart, rowStart + 2).map((item, idx) => (
              <Panel
                key={idx}
                direction="column"
                alignItems="center"
                flexGrow={1}
                flexBasis={0}
                padding={0}
                spacing={5}
              >
                <CircularProgress
                  value={item.percent}
                  size={52}
                  thickness={5.5}
                  color="#5C70E2"
                  trackColor="#DCE2F5"
                  textColor="#1E2538"
                  showText={true}
                  fontSize={11}
                />
                <Label
                  text={item.subject}
                  fontSize={9}
                  color="#475569"
                  horizontalAlign="center"
                />
              </Panel>
            ))}
          </Panel>
        ))}
      </Panel>
    </Panel>
  );
}
