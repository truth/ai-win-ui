export const Fragment = Symbol.for("ai-win-ui.Fragment");

export type ChildNode =
  | VNode
  | string
  | number
  | boolean
  | null
  | undefined
  | ChildNode[];

export type ComponentFn<P = any> = ((props: P) => ChildNode) & {
  __providerId?: symbol;
};

export interface VNode {
  $$vnode: true;
  type: string | symbol | ComponentFn;
  props: Record<string, any>;
  key: string | null;
}

export function jsx(
  type: string | symbol | ComponentFn,
  props: Record<string, any> | null,
  key?: string | number
): VNode {
  const safeProps = props ?? {};
  const resolvedKey =
    key !== undefined && key !== null
      ? String(key)
      : safeProps.key !== undefined && safeProps.key !== null
      ? String(safeProps.key)
      : null;
  return {
    $$vnode: true,
    type,
    props: safeProps,
    key: resolvedKey,
  };
}

export const jsxs = jsx;
export const jsxDEV = jsx;

export function createElement(
  type: string | symbol | ComponentFn,
  props: Record<string, any> | null,
  ...children: ChildNode[]
): VNode {
  const nextProps = props ? { ...props } : {};
  if (children.length === 1) {
    nextProps.children = children[0];
  } else if (children.length > 1) {
    nextProps.children = children;
  }
  return jsx(type, nextProps, nextProps.key);
}

