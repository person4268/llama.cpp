const char index_html[] = R"LITERAL(
<html>

<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1" />
  <meta name="color-scheme" content="light dark">
  <title>llama.cpp - chat</title>

  <style>
    body {
      font-family: system-ui;
      font-size: 90%;
    }

    #container {
      margin: 0em auto;
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      height: 100%;
    }

    main {
      margin: 3px;
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      gap: 1em;

      flex-grow: 1;
      overflow-y: auto;

      border: 1px solid #ccc;
      border-radius: 5px;
      padding: 0.5em;
    }

    body {
      max-width: 600px;
      min-width: 300px;
      line-height: 1.2;
      margin: 0 auto;
      padding: 0 0.5em;
    }

    p {
      overflow-wrap: break-word;
      word-wrap: break-word;
      hyphens: auto;
      margin-top: 0.5em;
      margin-bottom: 0.5em;
    }

    #write form {
      margin: 1em 0 0 0;
      display: flex;
      flex-direction: column;
      gap: 0.5em;
      align-items: stretch;
    }

    .right {
      display: flex;
      flex-direction: row;
      gap: 0.5em;
      justify-content: flex-end;
    }

    fieldset {
      border: none;
      padding: 0;
      margin: 0;
    }

    fieldset.two {
      display: grid;
      grid-template: "a a";
      gap: 1em;
    }

    fieldset.three {
      display: grid;
      grid-template: "a a a";
      gap: 1em;
    }

    details {
      border: 1px solid #aaa;
      border-radius: 4px;
      padding: 0.5em 0.5em 0;
      margin-top: 0.5em;
    }

    summary {
      font-weight: bold;
      margin: -0.5em -0.5em 0;
      padding: 0.5em;
      cursor: pointer;
    }

    details[open] {
      padding: 0.5em;
    }

    .prob-set {
      padding: 0.3em;
      border-bottom: 1px solid #ccc;
    }

    .popover-content {
      position: absolute;
      background-color: white;
      padding: 0.2em;
      box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
    }

    textarea {
      padding: 5px;
      flex-grow: 1;
      width: 100%;
    }

    pre code {
      display: block;
      background-color: #222;
      color: #ddd;
    }

    code {
      font-family: monospace;
      padding: 0.1em 0.3em;
      border-radius: 3px;
    }

    fieldset label {
      margin: 0.5em 0;
      display: block;
    }

    fieldset label.slim {
      margin: 0 0.5em;
      display: inline;
    }

    header,
    footer {
      text-align: center;
    }

    footer {
      font-size: 80%;
      color: #888;
    }

    .mode-chat textarea[name=prompt] {
      height: 4.5em;
    }

    .mode-completion textarea[name=prompt] {
      height: 10em;
    }

    [contenteditable] {
      display: inline-block;
      white-space: pre-wrap;
      outline: 0px solid transparent;
    }

    @keyframes loading-bg-wipe {
      0% {
        background-position: 0%;
      }

      100% {
        background-position: 100%;
      }
    }

    .loading {
      --loading-color-1: #eeeeee00;
      --loading-color-2: #eeeeeeff;
      background-size: 50% 100%;
      background-image: linear-gradient(90deg, var(--loading-color-1), var(--loading-color-2), var(--loading-color-1));
      animation: loading-bg-wipe 2s linear infinite;
    }

    @media (prefers-color-scheme: dark) {
      .loading {
        --loading-color-1: #22222200;
        --loading-color-2: #222222ff;
      }

      .popover-content {
        background-color: black;
      }
    }
  </style>

  <script type="module">
    import {
      html, h, signal, effect, computed, render, useSignal, useEffect, useRef, Component
    } from '/index.js';

    import { llama } from '/completion.js';
    import { SchemaConverter } from '/json-schema-to-grammar.mjs';
    let selected_image = false;
    var slot_id = -1;

    const session = signal({
      prompt: "This is a conversation between User and Llama, a friendly chatbot. Llama is helpful, kind, honest, good at writing, and never fails to answer any requests immediately and with precision.",
      template: "{{prompt}}\n\n{{history}}\n{{char}}:",
      historyTemplate: "{{name}}: {{message}}",
      transcript: [],
      type: "chat",  // "chat" | "completion"
      char: "Llama",
      user: "User",
      image_selected: ''
    })

    const params = signal({
      n_predict: 400,
      temperature: 0.7,
      repeat_last_n: 256, // 0 = disable penalty, -1 = context size
      repeat_penalty: 1.18, // 1.0 = disabled
      top_k: 40, // <= 0 to use vocab size
      top_p: 0.95, // 1.0 = disabled
      min_p: 0.05, // 0 = disabled
      tfs_z: 1.0, // 1.0 = disabled
      typical_p: 1.0, // 1.0 = disabled
      presence_penalty: 0.0, // 0.0 = disabled
      frequency_penalty: 0.0, // 0.0 = disabled
      mirostat: 0, // 0/1/2
      mirostat_tau: 5, // target entropy
      mirostat_eta: 0.1, // learning rate
      grammar: '',
      n_probs: 0, // no completion_probabilities,
      image_data: [],
      cache_prompt: true,
      api_key: ''
    })

    /* START: Support for storing prompt templates and parameters in browsers LocalStorage */

    const local_storage_storageKey = "llamacpp_server_local_storage";

    function local_storage_setDataFromObject(tag, content) {
      localStorage.setItem(local_storage_storageKey + '/' + tag, JSON.stringify(content));
    }

    function local_storage_setDataFromRawText(tag, content) {
      localStorage.setItem(local_storage_storageKey + '/' + tag, content);
    }

    function local_storage_getDataAsObject(tag) {
      const item = localStorage.getItem(local_storage_storageKey + '/' + tag);
      if (!item) {
        return null;
      } else {
        return JSON.parse(item);
      }
    }

    function local_storage_getDataAsRawText(tag) {
      const item = localStorage.getItem(local_storage_storageKey + '/' + tag);
      if (!item) {
        return null;
      } else {
        return item;
      }
    }

    // create a container for user templates and settings

    const savedUserTemplates = signal({})
    const selectedUserTemplate = signal({ name: '', template: { session: {}, params: {} } })

    // let's import locally saved templates and settings if there are any
    // user templates and settings are stored in one object
    // in form of { "templatename": "templatedata" } and { "settingstemplatename":"settingsdata" }

    console.log('Importing saved templates')

    let importedTemplates = local_storage_getDataAsObject('user_templates')

    if (importedTemplates) {
      // saved templates were successfully imported.

      console.log('Processing saved templates and updating default template')
      params.value = { ...params.value, image_data: [] };

      //console.log(importedTemplates);
      savedUserTemplates.value = importedTemplates;

      //override default template
      savedUserTemplates.value.default = { session: session.value, params: params.value }
      local_storage_setDataFromObject('user_templates', savedUserTemplates.value)
    } else {
      // no saved templates detected.

      console.log('Initializing LocalStorage and saving default template')

      savedUserTemplates.value = { "default": { session: session.value, params: params.value } }
      local_storage_setDataFromObject('user_templates', savedUserTemplates.value)
    }

    function userTemplateResetToDefault() {
      console.log('Resetting template to default')
      selectedUserTemplate.value.name = 'default';
      selectedUserTemplate.value.data = savedUserTemplates.value['default'];
    }

    function userTemplateApply(t) {
      session.value = t.data.session;
      session.value = { ...session.value, image_selected: '' };
      params.value = t.data.params;
      params.value = { ...params.value, image_data: [] };
    }

    function userTemplateResetToDefaultAndApply() {
      userTemplateResetToDefault()
      userTemplateApply(selectedUserTemplate.value)
    }

    function userTemplateLoadAndApplyAutosaved() {
      // get autosaved last used template
      let lastUsedTemplate = local_storage_getDataAsObject('user_templates_last')

      if (lastUsedTemplate) {

        console.log('Autosaved template found, restoring')

        selectedUserTemplate.value = lastUsedTemplate
      }
      else {

        console.log('No autosaved template found, using default template')
        // no autosaved last used template was found, so load from default.

        userTemplateResetToDefault()
      }

      console.log('Applying template')
      // and update internal data from templates

      userTemplateApply(selectedUserTemplate.value)
    }

    //console.log(savedUserTemplates.value)
    //console.log(selectedUserTemplate.value)

    function userTemplateAutosave() {
      console.log('Template Autosave...')
      if (selectedUserTemplate.value.name == 'default') {
        // we don't want to save over default template, so let's create a new one
        let newTemplateName = 'UserTemplate-' + Date.now().toString()
        let newTemplate = { 'name': newTemplateName, 'data': { 'session': session.value, 'params': params.value } }

        console.log('Saving as ' + newTemplateName)

        // save in the autosave slot
        local_storage_setDataFromObject('user_templates_last', newTemplate)

        // and load it back and apply
        userTemplateLoadAndApplyAutosaved()
      } else {
        local_storage_setDataFromObject('user_templates_last', { 'name': selectedUserTemplate.value.name, 'data': { 'session': session.value, 'params': params.value } })
      }
    }

    console.log('Checking for autosaved last used template')
    userTemplateLoadAndApplyAutosaved()

    /* END: Support for storing prompt templates and parameters in browsers LocalStorage */

    const llamaStats = signal(null)
    const controller = signal(null)

    // currently generating a completion?
    const generating = computed(() => controller.value != null)

    // has the user started a chat?
    const chatStarted = computed(() => session.value.transcript.length > 0)

    const transcriptUpdate = (transcript) => {
      session.value = {
        ...session.value,
        transcript
      }
    }

    // simple template replace
    const template = (str, extraSettings) => {
      let settings = session.value;
      if (extraSettings) {
        settings = { ...settings, ...extraSettings };
      }
      return String(str).replaceAll(/\{\{(.*?)\}\}/g, (_, key) => template(settings[key]));
    }

    async function runLlama(prompt, llamaParams, char) {
      const currentMessages = [];
      const history = session.value.transcript;
      if (controller.value) {
        throw new Error("already running");
      }
      controller.value = new AbortController();
      for await (const chunk of llama(prompt, llamaParams, { controller: controller.value })) {
        const data = chunk.data;

        if (data.stop) {
          while (
            currentMessages.length > 0 &&
            currentMessages[currentMessages.length - 1].content.match(/\n$/) != null
          ) {
            currentMessages.pop();
          }
          transcriptUpdate([...history, [char, currentMessages]])
          console.log("Completion finished: '", currentMessages.map(msg => msg.content).join(''), "', summary: ", data);
        } else {
          currentMessages.push(data);
          slot_id = data.slot_id;
          if (selected_image && !data.multimodal) {
            alert("The server was not compiled for multimodal or the model projector can't be loaded.");
            return;
          }
          transcriptUpdate([...history, [char, currentMessages]])
        }

        if (data.timings) {
          llamaStats.value = data;
        }
      }

      controller.value = null;
    }

    // send message to server
    const chat = async (msg) => {
      if (controller.value) {
        console.log('already running...');
        return;
      }

      transcriptUpdate([...session.value.transcript, ["{{user}}", msg]])

      let prompt = template(session.value.template, {
        message: msg,
        history: session.value.transcript.flatMap(
          ([name, data]) =>
            template(
              session.value.historyTemplate,
              {
                name,
                message: Array.isArray(data) ?
                  data.map(msg => msg.content).join('').replace(/^\s/, '') :
                  data,
              }
            )
        ).join("\n"),
      });
      if (selected_image) {
        prompt = `A chat between a curious human and an artificial intelligence assistant. The assistant gives helpful, detailed, and polite answers to the human's questions.\nUSER:[img-10]${msg}\nASSISTANT:`;
      }
      await runLlama(prompt, {
        ...params.value,
        slot_id: slot_id,
        stop: ["</s>", template("{{char}}:"), template("{{user}}:")],
      }, "{{char}}");
    }

    const runCompletion = () => {
      if (controller.value) {
        console.log('already running...');
        return;
      }
      const { prompt } = session.value;
      transcriptUpdate([...session.value.transcript, ["", prompt]]);
      runLlama(prompt, {
        ...params.value,
        slot_id: slot_id,
        stop: [],
      }, "").finally(() => {
        session.value.prompt = session.value.transcript.map(([_, data]) =>
          Array.isArray(data) ? data.map(msg => msg.content).join('') : data
        ).join('');
        session.value.transcript = [];
      })
    }

    const stop = (e) => {
      e.preventDefault();
      if (controller.value) {
        controller.value.abort();
        controller.value = null;
      }
    }

    const reset = (e) => {
      stop(e);
      transcriptUpdate([]);
    }

    const uploadImage = (e) => {
      e.preventDefault();
      document.getElementById("fileInput").click();
      document.getElementById("fileInput").addEventListener("change", function (event) {
        const selectedFile = event.target.files[0];
        if (selectedFile) {
          const reader = new FileReader();
          reader.onload = function () {
            const image_data = reader.result;
            session.value = { ...session.value, image_selected: image_data };
            params.value = {
              ...params.value, image_data: [
                { data: image_data.replace(/data:image\/[^;]+;base64,/, ''), id: 10 }]
            }
          };
          selected_image = true;
          reader.readAsDataURL(selectedFile);
        }
      });
    }

    function MessageInput() {
      const message = useSignal("")

      const submit = (e) => {
        stop(e);
        chat(message.value);
        message.value = "";
      }

      const enterSubmits = (event) => {
        if (event.which === 13 && !event.shiftKey) {
          submit(event);
        }
      }

      return html`
        <form onsubmit=${submit}>
          <div>
            <textarea
               className=${generating.value ? "loading" : null}
               oninput=${(e) => message.value = e.target.value}
               onkeypress=${enterSubmits}
               placeholder="Say something..."
               rows=2
               type="text"
               value="${message}"
            />
          </div>
          <div class="right">
            <button type="submit" disabled=${generating.value}>Send</button>
            <button onclick=${uploadImage}>Upload Image</button>
            <button onclick=${stop} disabled=${!generating.value}>Stop</button>
            <button onclick=${reset}>Reset</button>
          </div>
        </form>
      `
    }

    function CompletionControls() {
      const submit = (e) => {
        stop(e);
        runCompletion();
      }
      return html`
        <div>
          <button onclick=${submit} type="button" disabled=${generating.value}>Start</button>
          <button onclick=${stop} disabled=${!generating.value}>Stop</button>
          <button onclick=${reset}>Reset</button>
        </div>`;
    }

    const ChatLog = (props) => {
      const messages = session.value.transcript;
      const container = useRef(null)

      useEffect(() => {
        // scroll to bottom (if needed)
        const parent = container.current.parentElement;
        if (parent && parent.scrollHeight <= parent.scrollTop + parent.offsetHeight + 300) {
          parent.scrollTo(0, parent.scrollHeight)
        }
      }, [messages])

      const isCompletionMode = session.value.type === 'completion'
      const chatLine = ([user, data], index) => {
        let message
        const isArrayMessage = Array.isArray(data)
        if (params.value.n_probs > 0 && isArrayMessage) {
          message = html`<${Probabilities} data=${data} />`
        } else {
          const text = isArrayMessage ?
            data.map(msg => msg.content).join('').replace(/^\s+/, '') :
            data;
          message = isCompletionMode ?
            text :
            html`<${Markdownish} text=${template(text)} />`
        }
        if (user) {
          return html`<p key=${index}><strong>${template(user)}:</strong> ${message}</p>`
        } else {
          return isCompletionMode ?
            html`<span key=${index}>${message}</span>` :
            html`<p key=${index}>${message}</p>`
        }
      };

      const handleCompletionEdit = (e) => {
        session.value.prompt = e.target.innerText;
        session.value.transcript = [];
      }

      return html`
        <div id="chat" ref=${container} key=${messages.length}>
          <img style="width: 60%;${!session.value.image_selected ? `display: none;` : ``}" src="${session.value.image_selected}"/>
          <span contenteditable=${isCompletionMode} ref=${container} oninput=${handleCompletionEdit}>
            ${messages.flatMap(chatLine)}
          </span>
        </div>`;
    };

    const ConfigForm = (props) => {
      const updateSession = (el) => session.value = { ...session.value, [el.target.name]: el.target.value }
      const updateParams = (el) => params.value = { ...params.value, [el.target.name]: el.target.value }
      const updateParamsFloat = (el) => params.value = { ...params.value, [el.target.name]: parseFloat(el.target.value) }
      const updateParamsInt = (el) => params.value = { ...params.value, [el.target.name]: Math.floor(parseFloat(el.target.value)) }

      const grammarJsonSchemaPropOrder = signal('')
      const updateGrammarJsonSchemaPropOrder = (el) => grammarJsonSchemaPropOrder.value = el.target.value
      const convertJSONSchemaGrammar = () => {
        try {
          const schema = JSON.parse(params.value.grammar)
          const converter = new SchemaConverter(
            grammarJsonSchemaPropOrder.value
              .split(',')
              .reduce((acc, cur, i) => ({ ...acc, [cur.trim()]: i }), {})
          )
          converter.visit(schema, '')
          params.value = {
            ...params.value,
            grammar: converter.formatGrammar(),
          }
        } catch (e) {
          alert(`Convert failed: ${e.message}`)
        }
      }

      const FloatField = ({ label, max, min, name, step, value }) => {
        return html`
          <div>
            <label for="${name}">${label}</label>
            <input type="range" id="${name}" min="${min}" max="${max}" step="${step}" name="${name}" value="${value}" oninput=${updateParamsFloat} />
            <span>${value}</span>
          </div>
        `
      };

      const IntField = ({ label, max, min, name, value }) => {
        return html`
          <div>
            <label for="${name}">${label}</label>
            <input type="range" id="${name}" min="${min}" max="${max}" name="${name}" value="${value}" oninput=${updateParamsInt} />
            <span>${value}</span>
          </div>
        `
      };

      const userTemplateReset = (e) => {
        e.preventDefault();
        userTemplateResetToDefaultAndApply()
      }

      const UserTemplateResetButton = () => {
        if (selectedUserTemplate.value.name == 'default') {
          return html`
            <button disabled>Using default template</button>
          `
        }

        return html`
          <button onclick=${userTemplateReset}>Reset all to default</button>
        `
      };

      useEffect(() => {
        // autosave template on every change
        userTemplateAutosave()
      }, [session.value, params.value])

      const GrammarControl = () => (
        html`
          <div>
            <label for="template">Grammar</label>
            <textarea id="grammar" name="grammar" placeholder="Use gbnf or JSON Schema+convert" value="${params.value.grammar}" rows=4 oninput=${updateParams}/>
            <input type="text" name="prop-order" placeholder="order: prop1,prop2,prop3" oninput=${updateGrammarJsonSchemaPropOrder} />
            <button type="button" onclick=${convertJSONSchemaGrammar}>Convert JSON Schema</button>
          </div>
          `
      );

      const PromptControlFieldSet = () => (
        html`
        <fieldset>
          <div>
            <label htmlFor="prompt">Prompt</label>
            <textarea type="text" name="prompt" value="${session.value.prompt}" oninput=${updateSession}/>
          </div>
        </fieldset>
        `
      );

      const ChatConfigForm = () => (
        html`
          ${PromptControlFieldSet()}

          <fieldset class="two">
            <div>
              <label for="user">User name</label>
              <input type="text" name="user" value="${session.value.user}" oninput=${updateSession} />
            </div>

            <div>
              <label for="bot">Bot name</label>
              <input type="text" name="char" value="${session.value.char}" oninput=${updateSession} />
            </div>
          </fieldset>

          <fieldset>
            <div>
              <label for="template">Prompt template</label>
              <textarea id="template" name="template" value="${session.value.template}" rows=4 oninput=${updateSession}/>
            </div>

            <div>
              <label for="template">Chat history template</label>
              <textarea id="template" name="historyTemplate" value="${session.value.historyTemplate}" rows=1 oninput=${updateSession}/>
            </div>
            ${GrammarControl()}
          </fieldset>
      `
      );

      const CompletionConfigForm = () => (
        html`
          ${PromptControlFieldSet()}
          <fieldset>${GrammarControl()}</fieldset>
        `
      );

      return html`
        <form>
          <fieldset class="two">
            <${UserTemplateResetButton}/>
            <div>
              <label class="slim"><input type="radio" name="type" value="chat" checked=${session.value.type === "chat"} oninput=${updateSession} /> Chat</label>
              <label class="slim"><input type="radio" name="type" value="completion" checked=${session.value.type === "completion"} oninput=${updateSession} /> Completion</label>
            </div>
          </fieldset>

          ${session.value.type === 'chat' ? ChatConfigForm() : CompletionConfigForm()}

          <fieldset class="two">
            ${IntField({ label: "Predictions", max: 2048, min: -1, name: "n_predict", value: params.value.n_predict })}
            ${FloatField({ label: "Temperature", max: 2.0, min: 0.0, name: "temperature", step: 0.01, value: params.value.temperature })}
            ${FloatField({ label: "Penalize repeat sequence", max: 2.0, min: 0.0, name: "repeat_penalty", step: 0.01, value: params.value.repeat_penalty })}
            ${IntField({ label: "Consider N tokens for penalize", max: 2048, min: 0, name: "repeat_last_n", value: params.value.repeat_last_n })}
            ${IntField({ label: "Top-K sampling", max: 100, min: -1, name: "top_k", value: params.value.top_k })}
            ${FloatField({ label: "Top-P sampling", max: 1.0, min: 0.0, name: "top_p", step: 0.01, value: params.value.top_p })}
            ${FloatField({ label: "Min-P sampling", max: 1.0, min: 0.0, name: "min_p", step: 0.01, value: params.value.min_p })}
          </fieldset>
          <details>
            <summary>More options</summary>
            <fieldset class="two">
              ${FloatField({ label: "TFS-Z", max: 1.0, min: 0.0, name: "tfs_z", step: 0.01, value: params.value.tfs_z })}
              ${FloatField({ label: "Typical P", max: 1.0, min: 0.0, name: "typical_p", step: 0.01, value: params.value.typical_p })}
              ${FloatField({ label: "Presence penalty", max: 1.0, min: 0.0, name: "presence_penalty", step: 0.01, value: params.value.presence_penalty })}
              ${FloatField({ label: "Frequency penalty", max: 1.0, min: 0.0, name: "frequency_penalty", step: 0.01, value: params.value.frequency_penalty })}
            </fieldset>
            <hr />
            <fieldset class="three">
              <div>
                <label><input type="radio" name="mirostat" value="0" checked=${params.value.mirostat == 0} oninput=${updateParamsInt} /> no Mirostat</label>
                <label><input type="radio" name="mirostat" value="1" checked=${params.value.mirostat == 1} oninput=${updateParamsInt} /> Mirostat v1</label>
                <label><input type="radio" name="mirostat" value="2" checked=${params.value.mirostat == 2} oninput=${updateParamsInt} /> Mirostat v2</label>
              </div>
              ${FloatField({ label: "Mirostat tau", max: 10.0, min: 0.0, name: "mirostat_tau", step: 0.01, value: params.value.mirostat_tau })}
              ${FloatField({ label: "Mirostat eta", max: 1.0, min: 0.0, name: "mirostat_eta", step: 0.01, value: params.value.mirostat_eta })}
            </fieldset>
            <fieldset>
              ${IntField({ label: "Show Probabilities", max: 10, min: 0, name: "n_probs", value: params.value.n_probs })}
            </fieldset>
            <fieldset>
              <label for="api_key">API Key</label>
              <input type="text" name="api_key" value="${params.value.api_key}" placeholder="Enter API key" oninput=${updateParams} />
            </fieldset>
          </details>
        </form>
      `
    }

    const probColor = (p) => {
      const r = Math.floor(192 * (1 - p));
      const g = Math.floor(192 * p);
      return `rgba(${r},${g},0,0.3)`;
    }

    const Probabilities = (params) => {
      return params.data.map(msg => {
        const { completion_probabilities } = msg;
        if (
          !completion_probabilities ||
          completion_probabilities.length === 0
        ) return msg.content

        if (completion_probabilities.length > 1) {
          // Not for byte pair
          if (completion_probabilities[0].content.startsWith('byte: \\')) return msg.content

          const splitData = completion_probabilities.map(prob => ({
            content: prob.content,
            completion_probabilities: [prob]
          }))
          return html`<${Probabilities} data=${splitData} />`
        }

        const { probs, content } = completion_probabilities[0]
        const found = probs.find(p => p.tok_str === msg.content)
        const pColor = found ? probColor(found.prob) : 'transparent'

        const popoverChildren = html`
          <div class="prob-set">
            ${probs.map((p, index) => {
          return html`
                <div
                  key=${index}
                  title=${`prob: ${p.prob}`}
                  style=${{
              padding: '0.3em',
              backgroundColor: p.tok_str === content ? probColor(p.prob) : 'transparent'
            }}
                >
                  <span>${p.tok_str}: </span>
                  <span>${Math.floor(p.prob * 100)}%</span>
                </div>
              `
        })}
          </div>
        `

        return html`
          <${Popover} style=${{ backgroundColor: pColor }} popoverChildren=${popoverChildren}>
            ${msg.content.match(/\n/gim) ? html`<br />` : msg.content}
          </>
        `
      });
    }

    // poor mans markdown replacement
    const Markdownish = (params) => {
      const md = params.text
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/^#{1,6} (.*)$/gim, '<h3>$1</h3>')
        .replace(/\*\*(.*?)\*\*/g, '<strong>$1</strong>')
        .replace(/__(.*?)__/g, '<strong>$1</strong>')
        .replace(/\*(.*?)\*/g, '<em>$1</em>')
        .replace(/_(.*?)_/g, '<em>$1</em>')
        .replace(/```.*?\n([\s\S]*?)```/g, '<pre><code>$1</code></pre>')
        .replace(/`(.*?)`/g, '<code>$1</code>')
        .replace(/\n/gim, '<br />');
      return html`<span dangerouslySetInnerHTML=${{ __html: md }} />`;
    };

    const ModelGenerationInfo = (params) => {
      if (!llamaStats.value) {
        return html`<span/>`
      }
      return html`
        <span>
          ${llamaStats.value.tokens_predicted} predicted, ${llamaStats.value.tokens_cached} cached, ${llamaStats.value.timings.predicted_per_token_ms.toFixed()}ms per token, ${llamaStats.value.timings.predicted_per_second.toFixed(2)} tokens per second
        </span>
      `
    }

    // simple popover impl
    const Popover = (props) => {
      const isOpen = useSignal(false);
      const position = useSignal({ top: '0px', left: '0px' });
      const buttonRef = useRef(null);
      const popoverRef = useRef(null);

      const togglePopover = () => {
        if (buttonRef.current) {
          const rect = buttonRef.current.getBoundingClientRect();
          position.value = {
            top: `${rect.bottom + window.scrollY}px`,
            left: `${rect.left + window.scrollX}px`,
          };
        }
        isOpen.value = !isOpen.value;
      };

      const handleClickOutside = (event) => {
        if (popoverRef.current && !popoverRef.current.contains(event.target) && !buttonRef.current.contains(event.target)) {
          isOpen.value = false;
        }
      };

      useEffect(() => {
        document.addEventListener('mousedown', handleClickOutside);
        return () => {
          document.removeEventListener('mousedown', handleClickOutside);
        };
      }, []);

      return html`
        <span style=${props.style} ref=${buttonRef} onClick=${togglePopover}>${props.children}</span>
        ${isOpen.value && html`
          <${Portal} into="#portal">
            <div
              ref=${popoverRef}
              class="popover-content"
              style=${{
            top: position.value.top,
            left: position.value.left,
          }}
            >
              ${props.popoverChildren}
            </div>
          </${Portal}>
        `}
      `;
    };

    // Source: preact-portal (https://github.com/developit/preact-portal/blob/master/src/preact-portal.js)
    /** Redirect rendering of descendants into the given CSS selector */
    class Portal extends Component {
      componentDidUpdate(props) {
        for (let i in props) {
          if (props[i] !== this.props[i]) {
            return setTimeout(this.renderLayer);
          }
        }
      }

      componentDidMount() {
        this.isMounted = true;
        this.renderLayer = this.renderLayer.bind(this);
        this.renderLayer();
      }

      componentWillUnmount() {
        this.renderLayer(false);
        this.isMounted = false;
        if (this.remote && this.remote.parentNode) this.remote.parentNode.removeChild(this.remote);
      }

      findNode(node) {
        return typeof node === 'string' ? document.querySelector(node) : node;
      }

      renderLayer(show = true) {
        if (!this.isMounted) return;

        // clean up old node if moving bases:
        if (this.props.into !== this.intoPointer) {
          this.intoPointer = this.props.into;
          if (this.into && this.remote) {
            this.remote = render(html`<${PortalProxy} />`, this.into, this.remote);
          }
          this.into = this.findNode(this.props.into);
        }

        this.remote = render(html`
          <${PortalProxy} context=${this.context}>
            ${show && this.props.children || null}
          </${PortalProxy}>
        `, this.into, this.remote);
      }

      render() {
        return null;
      }
    }
    // high-order component that renders its first child if it exists.
    // used as a conditional rendering proxy.
    class PortalProxy extends Component {
      getChildContext() {
        return this.props.context;
      }
      render({ children }) {
        return children || null;
      }
    }

    function App(props) {

      return html`
        <div class="mode-${session.value.type}">
          <header>
            <h1>llama.cpp</h1>
          </header>

          <main id="content">
            <${chatStarted.value ? ChatLog : ConfigForm} />
          </main>

          <section id="write">
            <${session.value.type === 'chat' ? MessageInput : CompletionControls} />
          </section>

          <footer>
            <p><${ModelGenerationInfo} /></p>
            <p>Powered by <a href="https://github.com/ggerganov/llama.cpp">llama.cpp</a> and <a href="https://ggml.ai">ggml.ai</a>.</p>
          </footer>
        </div>
      `;
    }

    render(h(App), document.querySelector('#container'));
  </script>
</head>

<body>
  <div id="container">
    <input type="file" id="fileInput" accept="image/*" style="display: none;">
  </div>
  <div id="portal"></div>
</body>

</html>

)LITERAL";
unsigned int index_html_len = sizeof(index_html);
