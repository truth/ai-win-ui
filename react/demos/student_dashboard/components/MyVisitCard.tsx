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
      background="#FFFFFF"
      cornerRadius={18}
      padding={[20, 16, 20, 16]}
      borderColor="#E2E8F0"
      border={[1, 1, 1, 1]}
      width={255}
      minWidth={255}
      maxWidth={255}
    >
      {/* Title & Month Dropdown */}
      <Panel
        direction="row"
        alignItems="center"
        justifyContent="space-between"
        padding={0}
      >
        <Label text="My visit" fontSize={16} bold={true} color="#0F172A" />
        <Panel
          direction="row"
          alignItems="center"
          padding={[10, 4, 10, 4]}
          cornerRadius={12}
          background="#F8FAFC"
          borderColor="#E2E8F0"
          border={[1, 1, 1, 1]}
          spacing={4}
          onClick={onToggleMonth}
        >
          <Label text={selectedMonth} fontSize={11} color="#64748B" />
          <Icon name="chevron-down" size={12} color="#64748B" />
        </Panel>
      </Panel>

      <Panel height={16} padding={0} />

      {/* 2-Column x 3-Row Grid of Circular Progress Indicators */}
      <Panel
        direction="column"
        alignItems="stretch"
        spacing={14}
        padding={0}
      >
        {/* Row 1 */}
        <Panel direction="row" justifyContent="space-around" padding={0}>
          {data.slice(0, 2).map((item, idx) => (
            <Panel
              key={idx}
              direction="column"
              alignItems="center"
              width={100}
              padding={0}
            >
              <CircularProgress
                value={item.percent}
                size={60}
                thickness={5.5}
                color="#3751FE"
                trackColor="#EEF2F6"
                textColor="#0F172A"
                showText={true}
                fontSize={12}
              />
              <Panel height={6} padding={0} />
              <Label
                text={item.subject}
                fontSize={10}
                color="#64748B"
                horizontalAlign="center"
              />
            </Panel>
          ))}
        </Panel>

        {/* Row 2 */}
        <Panel direction="row" justifyContent="space-around" padding={0}>
          {data.slice(2, 4).map((item, idx) => (
            <Panel
              key={idx}
              direction="column"
              alignItems="center"
              width={100}
              padding={0}
            >
              <CircularProgress
                value={item.percent}
                size={60}
                thickness={5.5}
                color="#3751FE"
                trackColor="#EEF2F6"
                textColor="#0F172A"
                showText={true}
                fontSize={12}
              />
              <Panel height={6} padding={0} />
              <Label
                text={item.subject}
                fontSize={10}
                color="#64748B"
                horizontalAlign="center"
              />
            </Panel>
          ))}
        </Panel>

        {/* Row 3 */}
        <Panel direction="row" justifyContent="space-around" padding={0}>
          {data.slice(4, 6).map((item, idx) => (
            <Panel
              key={idx}
              direction="column"
              alignItems="center"
              width={100}
              padding={0}
            >
              <CircularProgress
                value={item.percent}
                size={60}
                thickness={5.5}
                color="#3751FE"
                trackColor="#EEF2F6"
                textColor="#0F172A"
                showText={true}
                fontSize={12}
              />
              <Panel height={6} padding={0} />
              <Label
                text={item.subject}
                fontSize={10}
                color="#64748B"
                horizontalAlign="center"
              />
            </Panel>
          ))}
        </Panel>
      </Panel>
    </Panel>
  );
}

